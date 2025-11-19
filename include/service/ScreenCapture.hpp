#pragma once

#include "detail/screen_capture/CalleeRenderDependencies.hpp"
#include "detail/screen_capture/DeviceInfo.hpp"
#include "event/TriggerScreenshot.hpp"
#include "logging/LoggingFactory.hpp"
#include "service/InitParameters.hpp"
#include "wrappers/graphics/StarBuffers/Buffer.hpp"
#include "wrappers/graphics/StarTextures/Texture.hpp"

#include <starlight/common/Handle.hpp>

#include <concepts>

namespace star::service
{
constexpr const std::string ScreenCaptureServiceCalleeTypeName()
{
    return "star::service::screen_capture::callee";
};

template <typename TCopyPolicy>
concept CopyPolicyLike =
    requires(TCopyPolicy c, detail::screen_capture::DeviceInfo &deviceInfo,
             detail::screen_capture::CalleeRenderDependencies &deps, const uint8_t &frameInFlightIndex) {
        { c.init(deviceInfo) } -> std::same_as<void>;
        { c.registerWithCommandBufferManager() } -> std::same_as<void>;
        { c.triggerSubmission(deps, frameInFlightIndex) } -> std::same_as<void>;
    };

template <typename TWorkerControllerPolicy>
concept WorkerPolicyLike =
    requires(TWorkerControllerPolicy c, star::job::tasks::write_image_to_disk::WriteImageTask task) {
        { c.addWriteTask(std::move(task)) } -> std::same_as<void>;
    };

template <typename TCreateDependenciesPolicy>
concept CreateDepsPolicyLike =
    requires(TCreateDependenciesPolicy c, detail::screen_capture::DeviceInfo &deviceInfo,
             StarTextures::Texture targetTexture, const Handle &commandBufferContainingTarget,
             const Handle &targetTextureReadySemaphore) {
        {
            c.create(deviceInfo, targetTexture, commandBufferContainingTarget, targetTextureReadySemaphore)
        } -> std::same_as<detail::screen_capture::CalleeRenderDependencies>;
    };
template <WorkerPolicyLike TWorkerControllerPolicy, typename TCreateDependenciesPolicy, CopyPolicyLike TCopyPolicy>
class ScreenCapture
{
  public:
    ScreenCapture(TWorkerControllerPolicy workerPolicy, TCreateDependenciesPolicy createDependenciesPolicy,
                  TCopyPolicy copyPolicy)
        : m_workerPolicy(std::move(workerPolicy)), m_createDependenciesPolicy(std::move(createDependenciesPolicy)),
          m_copyPolicy(std::move(copyPolicy))
    {
    }

    void init(const uint8_t &numFramesInFlight)
    {
        common::HandleTypeRegistry::instance().registerType(ScreenCaptureServiceCalleeTypeName());
        m_deviceInfo.numFramesInFlight = numFramesInFlight;

        registerWithEventBus();

        assert(m_deviceInfo.eventBus != nullptr);
        m_copyPolicy.init(m_deviceInfo);
        initCommandBuffer();
    }

    void setInitParameters(InitParameters &params)
    {
        m_deviceInfo =
            detail::screen_capture::DeviceInfo{.device = &params.device,
                                               .commandManager = &params.commandBufferManager,
                                               .surface = &params.surface,
                                               .eventBus = &params.eventBus,
                                               .semaphoreManager = params.graphicsManagers.semaphoreManager.get(),
                                               .taskManager = &params.taskManager,
                                               .currentFrameCounter = &params.currentFrameCounter};
    }

    void shutdown()
    {
        assert(m_deviceInfo.device != nullptr && "Device must be valid");
        cleanupDependencies(m_deviceInfo.device->getVulkanDevice());
    }

  private:
    TWorkerControllerPolicy m_workerPolicy;
    TCreateDependenciesPolicy m_createDependenciesPolicy;
    TCopyPolicy m_copyPolicy;

    Handle m_subscriberHandle;
    detail::screen_capture::DeviceInfo m_deviceInfo;
    detail::screen_capture::CalleeRenderDependencies m_calleeDependencyTracker;

    void eventCallback(const star::common::IEvent &e, bool &keepAlive)
    {
        const auto &screenEvent = static_cast<const event::TriggerScreenshot &>(e);
        assert(m_deviceInfo.taskManager != nullptr && "Task manager must be initialized");

        if (!screenEvent.getCalleeRegistration().isInitialized())
        {
            screenEvent.getCalleeRegistration().type =
                common::HandleTypeRegistry::instance().getTypeGuaranteedExist(ScreenCaptureServiceCalleeTypeName());
            screenEvent.getCalleeRegistration().id = 0;

            m_calleeDependencyTracker = m_createDependenciesPolicy.create(m_deviceInfo, screenEvent.getTexture(),
                                                                          screenEvent.getTargetCommandBuffer(),
                                                                          screenEvent.getTargetTextureReadySemaphore());
        }

        // need way to wait for commands to be submitted BEFORE telling worker to start?
        m_copyPolicy.triggerSubmission(m_calleeDependencyTracker, screenEvent.getFrameInFlight());

        m_workerPolicy.addWriteTask(
            job::tasks::write_image_to_disk::Create(screenEvent.getTexture(), screenEvent.getName()));

        keepAlive = true;
    }

    Handle *notificationFromEventBusGetHandle()
    {
        return &m_subscriberHandle;
    }

    void notificationFromEventBusDeleteHandle(const Handle &handle) {

    };

    void cleanupDependencies(vk::Device &device)
    {
        for (auto &buffer : m_calleeDependencyTracker.hostVisibleBuffers)
        {
            buffer.cleanupRender(device);
        }
    }

    void registerWithEventBus()
    {
        assert(m_deviceInfo.eventBus != nullptr);
        this->m_deviceInfo.eventBus->subscribe(
            star::event::TriggerScreenshotTypeName(),
            {[this](const star::common::IEvent &e, bool &keepAlive) { this->eventCallback(e, keepAlive); },
             [this]() -> Handle * { return this->notificationFromEventBusGetHandle(); },
             [this](const Handle &handle) { this->notificationFromEventBusDeleteHandle(handle); }});
    }

    void initCommandBuffer()
    {
        assert(m_deviceInfo.commandManager != nullptr);
        assert(m_deviceInfo.device != nullptr);

        m_copyPolicy.registerWithCommandBufferManager();
    }
};
} // namespace star::service