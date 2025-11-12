#pragma once

#include "Handle.hpp"
#include "core/MappedHandleContainer.hpp"
#include "core/device/DeviceContext.hpp"
#include "event/TriggerScreenshot.hpp"
#include "logging/LoggingFactory.hpp"
#include "service/InitParameters.hpp"
#include "wrappers/graphics/StarBuffers/Buffer.hpp"
#include "wrappers/graphics/StarTextures/Texture.hpp"

namespace star::service
{
template <typename TWorkerControllerPolicy, typename TCreateDependenciesPolicy> class ScreenCapture
{
  public:
    ScreenCapture(TWorkerControllerPolicy workerPolicy, TCreateDependenciesPolicy createDependenciesPolicy)
        : m_workerPolicy(std::move(workerPolicy)), m_createDependenciesPolicy(std::move(createDependenciesPolicy))
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

    struct CalleeRenderRequirenments
    {
        std::vector<StarTextures::Texture> m_transferDstTextures;
        std::vector<StarBuffers::Buffer> m_hostVisibleBuffers;
        std::vector<Handle> m_targetTexturesReadySemaphores;
    };

    TWorkerControllerPolicy m_workerPolicy;
    TCreateDependenciesPolicy m_createDependenciesPolicy;
    Handle m_subscriberHandle;
    std::vector<StarTextures::Texture> m_targetTextures;
    DeviceInfo m_deviceInfo;
    core::MappedHandleContainer<CalleeRenderRequirenments, star::Handle_Type::service_callee> m_calleeDependencyTracker;

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
        m_workerPolicy.addWriteTask();
        keepAlive = true;
    }

    Handle *notificationFromEventBusGetHandle()
    {
        return &m_subscriberHandle;
    }

    void notificationFromEventBusDeleteHandle(const Handle &handle){

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

    void recordCommandBuffer(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                             const uint64_t &frameIndex);

    void addMemoryDependencies(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                               const uint64_t &frameIndex) const;

    void registerWithEventBus(core::device::system::EventBus &eventBus)
    {
        eventBus.subscribe<event::TriggerScreenshot>(
            {[this](const star::common::IEvent &e, bool &keepAlive) { this->eventCallback(e, keepAlive); },
             [this]() -> Handle * { return this->notificationFromEventBusGetHandle(); },
             [this](const Handle &handle) { this->notificationFromEventBusDeleteHandle(handle); }});
    }
};
} // namespace star::service