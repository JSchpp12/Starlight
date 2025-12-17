#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include "ConfigFile.hpp"
#include "Enums.hpp"
#include "StarApplication.hpp"
#include "StarCommandBuffer.hpp"
#include "StarRenderGroup.hpp"
#include "StarScene.hpp"
#include "TransferWorker.hpp"
#include "core/SystemContext.hpp"
#include "core/logging/LoggingFactory.hpp"
#include "event/EnginePhaseComplete.hpp"
#include "event/RenderReadyForFinalization.hpp"
#include "service/ScreenCaptureFactory.hpp"

#include <star_common/HandleTypeRegistry.hpp>

#include <memory>
#include <string>
#include <vector>

#include <concepts>

namespace star
{

template <typename TEngineInitPolicy, typename TMainLoopPolicy, typename TEngineExitPolicy> class StarEngine
{
  public:
    StarEngine(TEngineInitPolicy initPolicy, TMainLoopPolicy loopPolicy, TEngineExitPolicy exitPolicy,
               StarApplication &application)
        : m_initPolicy(std::move(initPolicy)), m_loopPolicy(std::move(loopPolicy)), m_exitPolicy(std::move(exitPolicy)),
          m_application(application),
          m_renderingInstance(m_initPolicy.createRenderingInstance(ConfigFile::getSetting(Config_Settings::app_name))),
          m_systemManager(&m_renderingInstance)
    {
        m_defaultDevice = {
            .type = common::HandleTypeRegistry::instance().getType(common::special_types::DeviceTypeName).value(),
            .id = 0};
        core::logging::init();
        core::logging::log(boost::log::trivial::info, "Logger initialized");

        std::set<star::Rendering_Features> features;
        {
            bool setting = false;
            std::istringstream(ConfigFile::getSetting(star::Config_Settings::required_device_feature_shader_float64)) >>
                std::boolalpha >> setting;

            if (setting)
            {
                features.insert(star::Rendering_Features::shader_float64);
            }
        }

        std::set<Rendering_Device_Features> renderingFeatures{Rendering_Device_Features::timeline_semaphores};

        {
            uint8_t framesInFlight;
            int readFramesInFlight = std::stoi(ConfigFile::getSetting(Config_Settings::frames_in_flight));
            if (!common::helper::SafeCast(readFramesInFlight, framesInFlight))
            {
                throw std::runtime_error("Invalid number of frames in flight in config file");
            }
            m_loopPolicy.init(framesInFlight);
        }

        m_defaultDevice = m_systemManager.registerDevice(core::device::DeviceContext{
            m_initPolicy.createNewDevice(m_renderingInstance, features, renderingFeatures)});
        m_systemManager.getContext(m_defaultDevice)
            .init(m_defaultDevice, m_loopPolicy.getMaxNumOfFramesInFlight(),
                  m_initPolicy.getEngineRenderingResolution());

        // try and get a transfer queue from different queue fams
        {
            std::set<uint32_t> selectedFamilyIndices = std::set<uint32_t>();
            std::vector<StarQueue> transferWorkerQueues = std::vector<StarQueue>();

            const auto transferFams = this->m_systemManager.getContext(m_defaultDevice)
                                          .getDevice()
                                          .getQueueOwnershipTracker()
                                          .getQueueFamiliesWhichSupport(vk::QueueFlagBits::eTransfer);

            for (const auto &fam : transferFams)
            {
                if (selectedFamilyIndices.size() > 1)
                    break;

                if (fam != m_systemManager.getContext(m_defaultDevice)
                               .getDevice()
                               .getDefaultQueue(Queue_Type::Tgraphics)
                               .getParentQueueFamilyIndex() &&
                    fam != m_systemManager.getContext(m_defaultDevice)
                               .getDevice()
                               .getDefaultQueue(Queue_Type::Tcompute)
                               .getParentQueueFamilyIndex() &&
                    !selectedFamilyIndices.contains(fam))
                {
                    auto nQueue = m_systemManager.getContext(m_defaultDevice)
                                      .getDevice()
                                      .getQueueOwnershipTracker()
                                      .giveMeQueueWithProperties(vk::QueueFlagBits::eTransfer, false, fam);

                    if (nQueue.has_value())
                    {
                        transferWorkerQueues.push_back(nQueue.value());

                        selectedFamilyIndices.insert(fam);
                    }
                }
            }
        }

        m_application.init();
    }

    ~StarEngine()
    {
        StarObject::cleanupSharedResources(m_systemManager.getContext(m_defaultDevice));
    }

    void run()
    {
        std::shared_ptr<StarScene> currentScene = m_application.loadScene(m_systemManager.getContext(m_defaultDevice),
                                                                          m_loopPolicy.getMaxNumOfFramesInFlight());

        assert(currentScene && "Application must provide a proper instance of a scene object");
        m_systemManager.getContext(m_defaultDevice)
            .getEventBus()
            .emit(event::EnginePhaseComplete{event::Phase::init, event::GetEnginePhaseCompleteInitTypeName});

        currentScene->prepRender(m_systemManager.getContext(m_defaultDevice), m_loopPolicy.getMaxNumOfFramesInFlight());
        registerScreenshotService(m_systemManager.getContext(m_defaultDevice),
                                  m_loopPolicy.getMaxNumOfFramesInFlight());

        m_systemManager.getContext(m_defaultDevice)
            .getEventBus()
            .emit(event::EnginePhaseComplete{event::Phase::init, event::GetEnginePhaseCompleteLoadTypeName});

        while (!m_exitPolicy.shouldExit())
        {
            m_loopPolicy.frameUpdate();
            const uint8_t &frameInFlightIndex = m_loopPolicy.getCurrentFrameInFlightIndex();

            // check if any new objects have been added
            m_systemManager.getContext(m_defaultDevice).prepareForNextFrame(frameInFlightIndex);

            m_application.frameUpdate(m_systemManager, frameInFlightIndex);
            currentScene->frameUpdate(m_systemManager.getContext(m_defaultDevice), frameInFlightIndex);

            ManagerRenderResource::frameUpdate(m_systemManager.getContext(m_defaultDevice).getDeviceID(),
                                               frameInFlightIndex);
            vk::Semaphore allBuffersSubmitted =
                m_systemManager.getContext(m_defaultDevice)
                    .getManagerCommandBuffer()
                    .update(frameInFlightIndex, m_systemManager.getContext(m_defaultDevice).getCurrentFrameIndex());
            m_systemManager.getContext(m_defaultDevice)
                .getEventBus()
                .emit(event::RenderReadyForFinalization(m_systemManager.getContext(m_defaultDevice).getDevice()));

            this->m_systemManager.getContext(m_defaultDevice).getTransferWorker().update();
        }

        m_systemManager.getContext(m_defaultDevice).waitIdle();
        currentScene->cleanupRender(m_systemManager.getContext(m_defaultDevice));
        m_initPolicy.cleanup(m_renderingInstance.getVulkanInstance());
    }

  protected:
    TEngineInitPolicy m_initPolicy;
    TMainLoopPolicy m_loopPolicy;
    TEngineExitPolicy m_exitPolicy;

    StarApplication &m_application;
    core::RenderingInstance m_renderingInstance;
    core::SystemContext m_systemManager;
    Handle m_defaultDevice;
    uint64_t m_frameCounter = 0;
    std::shared_ptr<StarScene> currentScene = nullptr;

  private:
    void registerScreenshotService(core::device::DeviceContext &context, const uint8_t &numFramesInFlight)
    {
        m_systemManager.getContext(m_defaultDevice)
            .registerService(service::screen_capture::Builder(context.getDevice(), context.getTaskManager())
                                 .setNumWorkers(27)
                                 .build(),
                             numFramesInFlight);
    }
};
} // namespace star