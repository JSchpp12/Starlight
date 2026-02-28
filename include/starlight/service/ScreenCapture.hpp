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
#include "policy/command/ListenForGetScreenCaptureSyncInfo.hpp"
#include "service/InitParameters.hpp"
#include "starlight/core/waiter/sync_renderer/Factory.hpp"
#include "starlight/job/worker/DefaultWorker.hpp"
#include "starlight/job/worker/detail/default_worker/BusyWaitTaskHandlingPolicy.hpp"
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
    { c.triggerSubmission(copyPlan) } -> std::same_as<detail::screen_capture::GPUSynchronizationInfo>;
    { c.getCommandBuffer() } -> std::same_as<const star::Handle &>;
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
             vk::Semaphore targetTextureReadySemaphore) {
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
        : m_getSync(*this), m_workerPolicy(std::move(workerPolicy)),
          m_createDependenciesPolicy(std::move(createDependenciesPolicy)), m_copyPolicy(std::move(copyPolicy)),
          m_calleeDependencyTracker(star::service::detail::screen_capture::common::ScreenCaptureServiceCalleeTypeName)
    {
    }
    ScreenCapture(const ScreenCapture &) = delete;
    ScreenCapture &operator=(const ScreenCapture &) = delete;
    ScreenCapture(ScreenCapture &&other) noexcept
        : m_getSync(*this), m_workerPolicy(std::move(other.m_workerPolicy)),
          m_createDependenciesPolicy(std::move(other.m_createDependenciesPolicy)),
          m_copyPolicy(std::move(other.m_copyPolicy)),
          m_calleeDependencyTracker(std::move(other.m_calleeDependencyTracker)),
          m_subscriberHandle(std::move(other.m_subscriberHandle)), m_actionRouter(std::move(other.m_actionRouter)),
          m_deviceInfo(std::move(other.m_deviceInfo))
    {
        if (m_deviceInfo.cmdBus != nullptr)
        {
            other.m_getSync.cleanup(*m_deviceInfo.cmdBus);
            m_getSync.init(*m_deviceInfo.cmdBus);
        }
    }
    ScreenCapture &operator=(ScreenCapture &&other) noexcept
    {
        if (this != &other)
        {
            m_workerPolicy = std::move(other.m_workerPolicy);
            m_createDependenciesPolicy = std::move(other.m_createDependenciesPolicy);
            m_copyPolicy = std::move(other.m_copyPolicy);
            m_calleeDependencyTracker = std::move(other.m_calleeDependencyTracker);
            m_subscriberHandle = std::move(other.m_subscriberHandle);
            m_deviceInfo = std::move(other.m_deviceInfo);

            if (m_deviceInfo.cmdBus != nullptr)
            {
                other.cleanupDependencies(*m_deviceInfo.cmdBus);
                m_getSync.init(*m_deviceInfo.cmdBus);
            }
        }
        return *this;
    }
    ~ScreenCapture() = default;

    void negotiateWorkers(star::core::WorkerPool &pool, job::TaskManager &tm)
    {
        size_t numToCreate = 0;

        const int goal = pool.getNumAvailableWorkers() - 2; 
        {
            std::string msg = "Num workers for image capture: " + std::to_string(goal);
            star::core::info(msg);
        }

        if (goal < 0)
        {
            STAR_THROW("Not enough available workers to create screen capture service");
        }

        for (int i{0}; i < goal; i++)
        {
            if (pool.allocateWorker())
            {
                numToCreate++;
            }
        }

        m_workerPolicy.init(registerWorkers(numToCreate, tm));
    }

    void init()
    {
        assert(m_deviceInfo.eventBus != nullptr);
        assert(m_deviceInfo.commandManager != nullptr);

        m_getSync.init(*m_deviceInfo.cmdBus);
        m_actionRouter.init(&m_deviceInfo);
        m_copyPolicy.init(m_deviceInfo);
        registerWithEventBus();
        initCommandBuffer();
    }

    void setInitParameters(InitParameters &params)
    {
        m_deviceInfo =
            detail::screen_capture::DeviceInfo{.device = &params.device,
                                               .commandManager = &params.commandBufferManager,
                                               .eventBus = &params.eventBus,
                                               .semaphoreManager = params.graphicsManagers.semaphoreManager.get(),
                                               .queueManager = &params.graphicsManagers.queueManager,
                                               .taskManager = &params.taskManager,
                                               .flightTracker = &params.flightTracker,
                                               .cmdBus = &params.commandBus};
    }

    void shutdown()
    {
        assert(m_deviceInfo.device != nullptr && "Device must be valid");
        assert(m_deviceInfo.cmdBus != nullptr && "Command bus must be valid");

        m_getSync.cleanup(*m_deviceInfo.cmdBus);
        cleanupDependencies(*m_deviceInfo.device);
    }

    void onGetSyncInfo(command::GetScreenCaptureSyncInfo &cmd)
    {
        cmd.getReply().set(command::get_sync_info::SyncInfo{&m_copyPolicy.getCommandBuffer()});
    }

  private:
    policy::ListenForGetScreenCaptureSyncInfo<
        ScreenCapture<TWorkerControllerPolicy, TCreateDependenciesPolicy, TCopyPolicy>>
        m_getSync;
    TWorkerControllerPolicy m_workerPolicy;
    TCreateDependenciesPolicy m_createDependenciesPolicy;
    TCopyPolicy m_copyPolicy;

    core::LinearHandleContainer<detail::screen_capture::CalleeRenderDependencies, 5> m_calleeDependencyTracker;
    Handle m_subscriberHandle;
    detail::screen_capture::CopyRouter m_actionRouter;
    detail::screen_capture::DeviceInfo m_deviceInfo;

    std::vector<job::worker::Worker::WorkerConcept *> registerWorkers(const size_t &numToCreate, job::TaskManager &tm)
    {
        auto newWorkers = std::vector<job::worker::Worker::WorkerConcept *>(numToCreate);
        for (size_t i{0}; i < numToCreate; i++)
        {
            std::ostringstream oss;
            oss << "Image Writer_" << std::to_string(i);

            auto worker = tm.registerWorker(
                {job::worker::DefaultWorker{job::worker::default_worker::BusyWaitTaskHandlingPolicy<
                                                job::tasks::write_image_to_disk::WriteImageTask, 500>{true},
                                            oss.str()}},
                job::tasks::write_image_to_disk::WriteImageTypeName);

            auto *newWorker = tm.getWorker(worker);
            assert(newWorker != nullptr && "Worker was not properly created");

            newWorkers[i] = newWorker->getRawConcept();
        }

        return newWorkers;
    }

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
        createCallbackForSyncingMainRenderer(syncInfo.copyCommandBuffer, screenEvent.getTargetCommandBuffer(),
                                             syncInfo.timelineSemaphoreForMainCopyCommandsDone,
                                             m_deviceInfo.flightTracker->getCurrent().getGlobalFrameCounter(),
                                             syncInfo.signalValue);

        job::tasks::write_image_to_disk::WritePayload payload{
            .path = screenEvent.getPath(),
            .semaphore = syncInfo.timelineSemaphoreForMainCopyCommandsDone,
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

    void notificationFromEventBusDeleteHandle(const Handle &handle)
    {
        if (m_subscriberHandle == handle)
        {
            m_subscriberHandle = Handle();
        }
    };

    void cleanupDependencies(core::device::StarDevice &device)
    {
        (void)device; 
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

    void createCallbackForSyncingMainRenderer(Handle copyCommandBuffer, Handle targetCommandBuffer,
                                              vk::Semaphore signalSemaphore, uint64_t currentFrameCount,
                                              const uint64_t &signalValue)
    {
        assert(m_deviceInfo.eventBus && m_deviceInfo.commandManager);

        core::waiter::sync_renderer::Factory(*m_deviceInfo.eventBus, *m_deviceInfo.commandManager)
            .setWaitPipelineStage(vk::PipelineStageFlagBits::eFragmentShader)
            .setCreatedOnFrameCount(std::move(currentFrameCount))
            .setSemaphoreSignalValue(signalValue)
            .setSemaphore(std::move(signalSemaphore))
            .setTargetFrameInFlightIndex(m_deviceInfo.flightTracker->getCurrent().getFrameInFlightIndex())
            .setSourceCommandBuffer(std::move(copyCommandBuffer))
            .setTargetCommandBuffer(std::move(targetCommandBuffer))
            .build();
    }
};
} // namespace star::service