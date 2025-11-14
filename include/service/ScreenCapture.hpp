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
concept CopyPolicyLike =
    requires(TCopyPolicy c, detail::screen_capture::CalleeRenderDependencies &deps, vk::CommandBuffer &commandBuffer,
             const uint8_t &frameInFlightIndex, const uint64_t &frameIndex) {
        { c.recordCommandBuffer(deps, commandBuffer, frameInFlightIndex, frameIndex) } -> std::same_as<void>;
        { c.addMemoryDependencies(deps, commandBuffer, frameInFlightIndex, frameIndex) } -> std::same_as<void>;
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
        assert(m_deviceInfo.eventBus != nullptr);
        registerWithEventBus(*m_deviceInfo.eventBus);
    }

    void setInitParameters(InitParameters &params)
    {
        m_deviceInfo = DeviceInfo{.device = &params.device,
                                  .surface = &params.surface,
                                  .eventBus = &params.eventBus,
                                  .taskManager = &params.taskManager};
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
        core::RenderingSurface *surface = nullptr;
        core::device::system::EventBus *eventBus = nullptr;
        job::TaskManager *taskManager = nullptr;
    };

    TWorkerControllerPolicy m_workerPolicy;
    TCreateDependenciesPolicy m_createDependenciesPolicy;
    TCopyPolicy m_copyPolicy;

    Handle m_subscriberHandle;
    std::vector<StarTextures::Texture> m_targetTextures;
    DeviceInfo m_deviceInfo;
    core::MappedHandleContainer<detail::screen_capture::CalleeRenderDependencies, star::Handle_Type::service_callee>
        m_calleeDependencyTracker;

    virtual Handle registerCommandBuffer(core::device::DeviceContext &context, const uint8_t &numFramesInFlight)
    {
        return Handle();
    }

    void trigger()
    {
        core::logging::log(boost::log::trivial::info, "Trigger screenshot");
    }

    void eventCallback(const star::common::IEvent &e, bool &keepAlive)
    {
        const auto &screenEvent = static_cast<const event::TriggerScreenshot &>(e);
        assert(m_deviceInfo.taskManager != nullptr && "Task manager must be initialized");
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

    void registerWithEventBus(core::device::system::EventBus &eventBus)
    {
        eventBus.subscribe<event::TriggerScreenshot>(
            {[this](const star::common::IEvent &e, bool &keepAlive) { this->eventCallback(e, keepAlive); },
             [this]() -> Handle * { return this->notificationFromEventBusGetHandle(); },
             [this](const Handle &handle) { this->notificationFromEventBusDeleteHandle(handle); }});
    }
};
} // namespace star::service