#pragma once

#include "ManagedHandleContainer.hpp"
#include "detail/screen_capture/CalleeRenderDependencies.hpp"
#include "detail/screen_capture/CapabilityCache.hpp"
#include "detail/screen_capture/Common.hpp"
#include "detail/screen_capture/CopyRouter.hpp"
#include "detail/screen_capture/DeviceInfo.hpp"
#include "detail/screen_capture/GPUSynchronizationInfo.hpp"
#include "event/TriggerScreenshot.hpp"
#include "job/tasks/TaskFactory.hpp"
#include "logging/LoggingFactory.hpp"
#include "service/InitParameters.hpp"
#include "wrappers/graphics/StarBuffers/Buffer.hpp"
#include "wrappers/graphics/StarTextures/Texture.hpp"

#include <star_common/Handle.hpp>

#include <concepts>

namespace star::service
{

template <typename TCopyPolicy>
concept CopyPolicyLike = requires(TCopyPolicy c, detail::screen_capture::DeviceInfo &deviceInfo,
                                  detail::screen_capture::CopyPlan &copyPlan, const Handle &calleeHandle) {
    { c.init(deviceInfo) } -> std::same_as<void>;
    { c.registerWithCommandBufferManager() } -> std::same_as<void>;
    { c.triggerSubmission(copyPlan) } -> std::same_as<detail::screen_capture::GPUSynchronizationInfo>;
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
             const Handle *targetTextureReadySemaphore) {
        {
            c.create(deviceInfo, targetTexture, commandBufferContainingTarget, targetTextureReadySemaphore)
        } -> std::same_as<detail::screen_capture::CalleeRenderDependencies>;
    };
/**
 * @brief only works with image attached to swap chain --> limitation caused by frame in flight tracking.
 * Using the targetImageIndex means that the copy will create num of dependencies matching the number of frames used in
 * the swapChain or the target finalization renderer. If some image is targeted from a smaller value of frame in
 * flight, such as an offscreen renderer which only has a number of images matching the num of frames in flight there
 * might be some bugs
 *
 * @tparam TWorkerControllerPolicy
 * @tparam TCreateDependenciesPolicy
 * @tparam TCopyPolicy
 */
template <WorkerPolicyLike TWorkerControllerPolicy, typename TCreateDependenciesPolicy, CopyPolicyLike TCopyPolicy>
class ScreenCapture
{
  public:
    ScreenCapture(TWorkerControllerPolicy workerPolicy, TCreateDependenciesPolicy createDependenciesPolicy,
                  TCopyPolicy copyPolicy)
        : m_workerPolicy(std::move(workerPolicy)), m_createDependenciesPolicy(std::move(createDependenciesPolicy)),
          m_copyPolicy(std::move(copyPolicy)),
          m_calleeDependencyTracker(star::service::detail::screen_capture::common::ScreenCaptureServiceCalleeTypeName)
    {
    }

    void init(const uint8_t &numFramesInFlight)
    {
        m_actionRouter.init(&m_deviceInfo);

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
                                               .eventBus = &params.eventBus,
                                               .semaphoreManager = params.graphicsManagers.semaphoreManager.get(),
                                               .taskManager = &params.taskManager,
                                               .flightTracker = &params.flightTracker};
    }

    void shutdown()
    {
        assert(m_deviceInfo.device != nullptr && "Device must be valid");
        cleanupDependencies(*m_deviceInfo.device);
    }

  private:
    TWorkerControllerPolicy m_workerPolicy;
    TCreateDependenciesPolicy m_createDependenciesPolicy;
    TCopyPolicy m_copyPolicy;
    core::LinearHandleContainer<detail::screen_capture::CalleeRenderDependencies, 5> m_calleeDependencyTracker;

    Handle m_subscriberHandle;
    detail::screen_capture::CopyRouter m_actionRouter;
    detail::screen_capture::DeviceInfo m_deviceInfo;

    void eventCallback(const star::common::IEvent &e, bool &keepAlive)
    {
        const auto &screenEvent = static_cast<const event::TriggerScreenshot &>(e);
        assert(m_deviceInfo.taskManager != nullptr && "Task manager must be initialized");

        if (!screenEvent.getCalleeRegistration().isInitialized())
        {
            auto newDeps = m_createDependenciesPolicy.create(m_deviceInfo, screenEvent.getTexture(),
                                                             screenEvent.getTargetCommandBuffer(),
                                                             screenEvent.getTargetTextureReadySemaphore());
            auto newHandle = m_calleeDependencyTracker.insert(std::move(newDeps));

            screenEvent.getCalleeRegistration() = newHandle;
        }

        auto copyPlan = m_actionRouter.decide(m_calleeDependencyTracker.get(screenEvent.getCalleeRegistration()),
                                              screenEvent.getCalleeRegistration(),
                                              m_deviceInfo.flightTracker->getCurrent().getFinalTargetImageIndex());

        // need way to wait for commands to be submitted BEFORE telling worker to start?
        detail::screen_capture::GPUSynchronizationInfo syncInfo = m_copyPolicy.triggerSubmission(copyPlan);

        uint64_t signalValue;
        common::helper::SafeCast(syncInfo.signalValue, signalValue);
        job::tasks::write_image_to_disk::WritePayload payload{
            .path = screenEvent.getName(),
            .semaphore = syncInfo.semaphore,
            .device = m_deviceInfo.device->getVulkanDevice(),
            .bufferImageInfo = std::make_unique<job::tasks::write_image_to_disk::BufferImageInfo>(
                copyPlan.resources.bufferInfo.containerRegistration,
                copyPlan.calleeDependencies->targetTexture.getBaseExtent(),
                copyPlan.calleeDependencies->targetTexture.getBaseFormat(),
                &copyPlan.resources.bufferInfo.container->getBufferPool()),
            .signalValue = std::make_unique<uint64_t>(std::move(signalValue))};
        m_workerPolicy.addWriteTask(job::tasks::write_image_to_disk::Create(std::move(payload)));

        keepAlive = true;
    }

    Handle *notificationFromEventBusGetHandle()
    {
        return &m_subscriberHandle;
    }

    void notificationFromEventBusDeleteHandle(const Handle &handle) {

    };

    void cleanupDependencies(core::device::StarDevice &device)
    {
        m_actionRouter.cleanupRender(&m_deviceInfo);
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