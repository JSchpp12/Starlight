#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include "ConfigFile.hpp"
#include "Enums.hpp"
#include "RenderingInstance.hpp"
#include "StarApplication.hpp"
#include "StarCommandBuffer.hpp"
#include "StarRenderGroup.hpp"
#include "StarScene.hpp"
#include "TransferWorker.hpp"
#include "core/SystemContext.hpp"
#include "core/logging/LoggingFactory.hpp"
#include "event/EnginePhaseComplete.hpp"
#include "renderer/SwapChainRenderer.hpp"
#include "service/ScreenCaptureFactory.hpp"

#include <starlight/common/HandleTypeRegistry.hpp>

#include <memory>
#include <string>
#include <vector>

namespace star
{

template <typename TFinalRenderControlPolicy> class StarEngine
{
  public:
    StarEngine(TFinalRenderControlPolicy finalRenderControlPolicy, StarApplication &application)
        : m_finalRenderPolicy(std::move(finalRenderControlPolicy)), m_application(application),
          m_renderingInstance(ConfigFile::getSetting(star::Config_Settings::app_name)),
          m_systemManager(&m_renderingInstance)
    {
        defaultDevice = {
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

        uint8_t framesInFlight;
        {
            int readFramesInFlight = std::stoi(ConfigFile::getSetting(Config_Settings::frames_in_flight));
            if (!common::helper::SafeCast<int, uint8_t>(readFramesInFlight, framesInFlight))
            {
                throw std::runtime_error("Invalid number of frames in flight in config file");
            }
        }

        m_finalRenderPolicy.init(application);

        m_systemManager.createDevice(defaultDevice, frameCounter, framesInFlight, features,
                                     *m_finalRenderPolicy.m_window, renderingFeatures);

        // try and get a transfer queue from different queue fams
        {
            std::set<uint32_t> selectedFamilyIndices = std::set<uint32_t>();
            std::vector<StarQueue> transferWorkerQueues = std::vector<StarQueue>();

            const auto transferFams = this->m_systemManager.getContext(defaultDevice)
                                          .getDevice()
                                          .getQueueOwnershipTracker()
                                          .getQueueFamiliesWhichSupport(vk::QueueFlagBits::eTransfer);

            for (const auto &fam : transferFams)
            {
                if (selectedFamilyIndices.size() > 1)
                    break;

                if (fam != m_systemManager.getContext(defaultDevice)
                               .getDevice()
                               .getDefaultQueue(Queue_Type::Tgraphics)
                               .getParentQueueFamilyIndex() &&
                    fam != m_systemManager.getContext(defaultDevice)
                               .getDevice()
                               .getDefaultQueue(Queue_Type::Tcompute)
                               .getParentQueueFamilyIndex() &&
                    !selectedFamilyIndices.contains(fam))
                {
                    auto nQueue = m_systemManager.getContext(defaultDevice)
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
        StarObject::cleanupSharedResources(m_systemManager.getContext(defaultDevice));
    }

    void run()
    {
        uint8_t numFramesInFlight;
        {
            int framesInFlight = std::stoi(ConfigFile::getSetting(Config_Settings::frames_in_flight));
            if (!star::common::helper::SafeCast<int, uint8_t>(framesInFlight, numFramesInFlight))
            {
                throw std::runtime_error("Failed to process number of frames in flight");
            }
        }

        std::shared_ptr<StarScene> currentScene = m_application.loadScene(
            m_systemManager.getContext(defaultDevice), *this->m_finalRenderPolicy.m_window, numFramesInFlight);

        assert(currentScene && "Application must provide a proper instance of a scene object");
        m_systemManager.getContext(defaultDevice)
            .getEventBus()
            .emit(event::EnginePhaseComplete{event::Phase::init, event::GetEnginePhaseCompleteInitTypeName});

        currentScene->prepRender(m_systemManager.getContext(defaultDevice), numFramesInFlight);
        registerScreenshotService(m_systemManager.getContext(defaultDevice), numFramesInFlight);

        m_systemManager.getContext(defaultDevice)
            .getEventBus()
            .emit(event::EnginePhaseComplete{event::Phase::init, event::GetEnginePhaseCompleteLoadTypeName});

        uint8_t frameInFlightIndex = 0;
        auto *swapChainRenderer = currentScene->getPresentationRenderer().getRaw<core::renderer::SwapChainRenderer>();
        while (!m_finalRenderPolicy.m_window->shouldClose())
        {
            frameInFlightIndex = swapChainRenderer->getFrameToBeDrawn();

            // check if any new objects have been added
            m_systemManager.getContext(defaultDevice).prepareForNextFrame(frameInFlightIndex);

            currentScene->getPresentationRenderer().frameUpdate(m_systemManager.getContext(defaultDevice),
                                                                frameInFlightIndex);
            InteractionSystem::callWorldUpdates(frameInFlightIndex);
            m_application.frameUpdate(m_systemManager, frameInFlightIndex);
            currentScene->frameUpdate(m_systemManager.getContext(defaultDevice), frameInFlightIndex);

            ManagerRenderResource::frameUpdate(m_systemManager.getContext(defaultDevice).getDeviceID(),
                                               frameInFlightIndex);
            vk::Semaphore allBuffersSubmitted =
                m_systemManager.getContext(defaultDevice)
                    .getManagerCommandBuffer()
                    .update(frameInFlightIndex, m_systemManager.getContext(defaultDevice).getCurrentFrameIndex());
            swapChainRenderer->submitPresentation(frameInFlightIndex, &allBuffersSubmitted);
            this->m_systemManager.getContext(defaultDevice).getTransferWorker().update();
        }

        m_systemManager.getContext(defaultDevice).waitIdle();
        currentScene->cleanupRender(m_systemManager.getContext(defaultDevice));
    }

  protected:
    TFinalRenderControlPolicy m_finalRenderPolicy;
    StarApplication &m_application;
    core::RenderingInstance m_renderingInstance;
    Handle defaultDevice;
    core::SystemContext m_systemManager;
    uint64_t frameCounter = 0;
    std::shared_ptr<StarScene> currentScene = nullptr;

  private:
    void registerScreenshotService(core::device::DeviceContext &context, const uint8_t &numFramesInFlight)
    {
        m_systemManager.getContext(defaultDevice)
            .registerService(service::screen_capture::Builder(context.getDevice(), context.getTaskManager())
                                 .setNumWorkers(27)
                                 .build(),
                             numFramesInFlight);
    }
};
} // namespace star