#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include "ConfigFile.hpp"
#include "StarApplication.hpp"
#include "StarCommandBuffer.hpp"
#include "StarRenderGroup.hpp"
#include "StarScene.hpp"
#include "TransferWorker.hpp"
#include "core/SystemContext.hpp"
#include "core/logging/LoggingFactory.hpp"
#include "event/EnginePhaseComplete.hpp"
#include "event/FrameComplete.hpp"
#include "event/RenderReadyForFinalization.hpp"
#include "job/worker/DefaultWorker.hpp"
#include "job/worker/detail/default_worker/SleepWaitTaskHandlingPolicy.hpp"
#include "service/ScreenCaptureFactory.hpp"
#include "starlight/service/IOService.hpp"
#include "tasks/IOTask.hpp"
#include "util/log/SystemInfo.hpp"

#include <star_common/FrameTracker.hpp>
#include <star_common/HandleTypeRegistry.hpp>

#include <concepts>
#include <memory>
#include <string>
#include <vector>

namespace star
{
template <typename T>
concept InitLike =
    requires(T init, std::string appName, core::RenderingInstance &instance, core::device::StarDevice &device,
             const uint8_t &numFramesInFlight, std::set<star::Rendering_Features> &features,
             std::set<Rendering_Device_Features> &deviceFeatures) {
        { init.createRenderingInstance(appName) } -> std::same_as<core::RenderingInstance>;
        { init.createNewDevice(instance, features, deviceFeatures) } -> std::same_as<core::device::StarDevice>;
        { init.init(numFramesInFlight) } -> std::same_as<void>;
        { init.cleanup(instance) } -> std::same_as<void>;
        { init.getFrameInFlightTrackingSetup(device) } -> std::same_as<common::FrameTracker::Setup>;
        { init.getEngineRenderingResolution() } -> std::same_as<vk::Extent2D>;
        { init.getAdditionalDeviceServices() } -> std::same_as<std::vector<service::Service>>;
    };

template <typename T>
concept ExitLike = requires(T exit) {
    { exit.shouldExit() } -> std::same_as<bool>;
};

template <typename T>
concept LoopLike = requires(T loop) {
    { loop.frameUpdate() } -> std::same_as<void>;
};
template <InitLike TEngineInitPolicy, LoopLike TMainLoopPolicy, ExitLike TEngineExitPolicy> class StarEngine
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
        star::log::logSystemOverview();

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
            m_initPolicy.init(framesInFlight);
        }

        m_defaultDevice = m_systemManager.registerDevice(core::device::DeviceContext{
            m_initPolicy.createNewDevice(m_renderingInstance, features, renderingFeatures)});
        m_systemManager.getContext(m_defaultDevice)
            .init(m_defaultDevice,
                  m_initPolicy.getFrameInFlightTrackingSetup(m_systemManager.getContext(m_defaultDevice).getDevice()),
                  m_initPolicy.getEngineRenderingResolution());

        {
            std::vector<service::Service> additionalServices = m_initPolicy.getAdditionalDeviceServices();
            for (size_t i{0}; i < additionalServices.size(); i++)
            {
                m_systemManager.getContext(m_defaultDevice).registerService(std::move(additionalServices[i]));
            }
        }
        registerScreenshotService(m_systemManager.getContext(m_defaultDevice));
        registerIOService(m_systemManager.getContext(m_defaultDevice));

        m_application.init();
    }

    ~StarEngine()
    {
        StarObject::cleanupSharedResources(m_systemManager.getContext(m_defaultDevice));
    }

    void run()
    {
        std::shared_ptr<StarScene> currentScene = m_application.loadScene(
            m_systemManager.getContext(m_defaultDevice),
            m_systemManager.getContext(m_defaultDevice).getFrameTracker().getSetup().getNumFramesInFlight());

        assert(currentScene && "Application must provide a proper instance of a scene object");
        m_systemManager.getContext(m_defaultDevice)
            .getEventBus()
            .emit(event::EnginePhaseComplete{event::Phase::init, event::GetEnginePhaseCompleteInitTypeName});

        currentScene->prepRender(m_systemManager.getContext(m_defaultDevice),
                                 m_systemManager.getContext(m_defaultDevice).getFrameTracker().getSetup());

        m_systemManager.getContext(m_defaultDevice)
            .getEventBus()
            .emit(event::EnginePhaseComplete{event::Phase::init, event::GetEnginePhaseCompleteLoadTypeName});

        while (!m_exitPolicy.shouldExit())
        {
            m_loopPolicy.frameUpdate();

            // check if any new objects have been added
            m_systemManager.getContext(m_defaultDevice).prepareForNextFrame();

            m_application.frameUpdate(m_systemManager);
            currentScene->frameUpdate(
                m_systemManager.getContext(m_defaultDevice),
                m_systemManager.getContext(m_defaultDevice).getFrameTracker().getCurrent().getFrameInFlightIndex());

            ManagerRenderResource::frameUpdate(
                m_systemManager.getContext(m_defaultDevice).getDeviceID(),
                m_systemManager.getContext(m_defaultDevice).getFrameTracker().getCurrent().getFrameInFlightIndex());
            vk::Semaphore allBuffersSubmitted =
                m_systemManager.getContext(m_defaultDevice)
                    .getManagerCommandBuffer()
                    .update(m_systemManager.getContext(m_defaultDevice).getFrameTracker());
            m_systemManager.getContext(m_defaultDevice)
                .getEventBus()
                .emit(event::RenderReadyForFinalization(m_systemManager.getContext(m_defaultDevice).getDevice(),
                                                        allBuffersSubmitted));

            this->m_systemManager.getContext(m_defaultDevice).getTransferWorker().update();

            m_systemManager.getContext(m_defaultDevice).getEventBus().emit(star::event::FrameComplete{});
        }

        m_systemManager.getContext(m_defaultDevice).waitIdle();
        m_application.shutdown(m_systemManager.getContext(m_defaultDevice));
        currentScene->cleanupRender(m_systemManager.getContext(m_defaultDevice));
        // need to cleanup services first
        m_systemManager.getContext(m_defaultDevice).cleanupRender();
        m_initPolicy.cleanup(m_renderingInstance); // destroy surface
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
    void registerScreenshotService(core::device::DeviceContext &context) const
    {
        context.registerService(
            service::screen_capture::Builder(context.getDevice(), context.getTaskManager()).setNumWorkers(26).build());
    }

    void registerIOService(core::device::DeviceContext &context) const
    {
        const std::string workerName = "IOWorker";
        Handle wHandle;
        {
            // star::job::worker::Worker worker;

            wHandle = context.getTaskManager().registerWorker(
                {star::job::worker::DefaultWorker(
                    job::worker::default_worker::SleepWaitTaskHandlingPolicy<job::tasks::io::IOTask, 64>{true},
                    workerName)},
                job::tasks::io::IOTaskName);
        }

        // context.registerService(service::Service{service::IOService(context.getTaskManager().getWorker(worker))});
    }
};
} // namespace star