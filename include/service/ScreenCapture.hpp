#pragma once

#include "Handle.hpp"
#include "core/MappedHandleContainer.hpp"
#include "core/device/DeviceContext.hpp"
#include "detail/screen_capture/CalleeRenderDependencies.hpp"
#include "event/TriggerScreenshot.hpp"
#include "logging/LoggingFactory.hpp"
#include "service/InitParameters.hpp"
#include "wrappers/graphics/StarBuffers/Buffer.hpp"
#include "wrappers/graphics/StarTextures/Texture.hpp"

#include <concepts>

namespace star::service
{
template <typename TCopyPolicy>
concept CopyPolicyLike = requires(TCopyPolicy c, core::device::manager::ManagerCommandBuffer &commandManager,
                                  core::device::system::EventBus &eventBus, core::device::StarDevice &device,
                                  detail::screen_capture::CalleeRenderDependencies deps, const uint64_t &frameIndex,
                                  const uint8_t &numFramesInFlight) {
    { c.init(eventBus, numFramesInFlight) } -> std::same_as<void>;
    { c.registerWithCommandBufferManager(device, commandManager, frameIndex) } -> std::same_as<void>;
    { c.registerCalleeDependency(deps) } -> std::same_as<void>;
    { c.triggerSubmission(commandManager) } -> std::same_as<void>;
};

template <typename TWorkerControllerPolicy>
concept WorkerPolicyLike =
    requires(TWorkerControllerPolicy c, star::job::tasks::write_image_to_disk::WriteImageTask task) {
        { c.addWriteTask(std::move(task)) } -> std::same_as<void>;
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
        registerWithEventBus();

        assert(m_deviceInfo.eventBus != nullptr);
        m_copyPolicy.init(*m_deviceInfo.eventBus, numFramesInFlight);
        initCommandBuffer();
    }

    void setInitParameters(InitParameters &params)
    {
        m_deviceInfo = DeviceInfo{.device = &params.device,
                                  .commandManager = &params.commandBufferManager,
                                  .surface = &params.surface,
                                  .eventBus = &params.eventBus,
                                  .taskManager = &params.taskManager,
                                  .currentFrameCounter = &params.currentFrameCounter};
    }

    void shutdown()
    {
        assert(m_deviceInfo.device != nullptr && "Device must be valid");
        cleanupIntermediateImages(*m_deviceInfo.device);
        cleanupBuffers(*m_deviceInfo.device);
    }

  private:
    struct DeviceInfo
    {
        core::device::StarDevice *device = nullptr;
        core::device::manager::ManagerCommandBuffer *commandManager = nullptr;
        core::RenderingSurface *surface = nullptr;
        core::device::system::EventBus *eventBus = nullptr;
        job::TaskManager *taskManager = nullptr;
        const uint64_t *currentFrameCounter = nullptr;
    };

    TWorkerControllerPolicy m_workerPolicy;
    TCreateDependenciesPolicy m_createDependenciesPolicy;
    TCopyPolicy m_copyPolicy;

    Handle m_subscriberHandle;
    std::vector<StarTextures::Texture> m_targetTextures;
    DeviceInfo m_deviceInfo;
    detail::screen_capture::CalleeRenderDependencies m_calleeDependencyTracker;

    void eventCallback(const star::common::IEvent &e, bool &keepAlive)
    {
        const auto &screenEvent = static_cast<const event::TriggerScreenshot &>(e);
        assert(m_deviceInfo.taskManager != nullptr && "Task manager must be initialized");

        // need way to wait for commands to be submitted BEFORE telling worker to start?
        m_copyPolicy.triggerSubmission(*m_deviceInfo.commandManager);

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

    void cleanupBuffers(core::device::StarDevice &device)
    {
        // for (auto &buffer : m_hostVisibleBuffers)
        // {
        //     buffer.cleanupRender(device.getVulkanDevice());
        // }
    }

    void cleanupIntermediateImages(core::device::StarDevice &device)
    {
        // for (auto &image : m_transferDstTextures)
        // {
        //     image.cleanupRender(device.getVulkanDevice());
        // }
    }

    void registerWithEventBus()
    {
        assert(m_deviceInfo.eventBus != nullptr);
        this->m_deviceInfo.eventBus->template subscribe<event::TriggerScreenshot>(
            {[this](const star::common::IEvent &e, bool &keepAlive) { this->eventCallback(e, keepAlive); },
             [this]() -> Handle * { return this->notificationFromEventBusGetHandle(); },
             [this](const Handle &handle) { this->notificationFromEventBusDeleteHandle(handle); }});
    }

    void initCommandBuffer()
    {
        assert(m_deviceInfo.commandManager != nullptr);
        assert(m_deviceInfo.device != nullptr);

        m_copyPolicy.registerWithCommandBufferManager(
            *m_deviceInfo.device, *m_deviceInfo.commandManager, *m_deviceInfo.currentFrameCounter);
    }
};
} // namespace star::service