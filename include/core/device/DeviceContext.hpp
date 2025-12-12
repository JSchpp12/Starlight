#pragma once

#include "RenderingSurface.hpp"
#include "SwapChainSupportDetails.hpp"
#include "TaskManager.hpp"
#include "core/device/FrameInFlightTracking.hpp"
#include "device/StarDevice.hpp"
#include "device/managers/GraphicsContainer.hpp"
#include "device/managers/ManagerCommandBuffer.hpp"
#include "device/managers/Pipeline.hpp"
#include "service/Service.hpp"
#include "tasks/TaskFactory.hpp"

#include <starlight/common/IDeviceContext.hpp>

#include <vulkan/vulkan.hpp>

#include <memory>
#include <vector>

namespace star::core::device
{
class DeviceContext : public star::common::IDeviceContext
{
  public:
    struct ManagerCommandBufferWrapper
    {
        Handle submit(device::manager::ManagerCommandBuffer::Request request, const uint64_t &frameIndex)
        {
            return m_manager.submit(m_device, frameIndex, request);
        }

        vk::Semaphore update(const uint8_t &frameInFlightIndex, const uint64_t &frameIndex)
        {
            return m_manager.update(m_device, frameInFlightIndex, frameIndex);
        }

        StarDevice &m_device;
        manager::ManagerCommandBuffer &m_manager;
    };
    DeviceContext() = default;
    virtual ~DeviceContext();
    DeviceContext(DeviceContext &&other);
    DeviceContext &operator=(DeviceContext &&other);
    DeviceContext(const DeviceContext &) = delete;
    DeviceContext &operator=(const DeviceContext &) = delete;

    void init(const Handle &deviceID, const uint8_t &numFramesInFlight, RenderingInstance &instance,
              std::set<Rendering_Features> requiredFeatures, StarWindow &window,
              const std::set<Rendering_Device_Features> &requiredRenderingDeviceFeatures);

    void waitIdle();

    void prepareForNextFrame(const uint8_t &frameInFlightIndex);

    inline StarDevice &getDevice()
    {
        return *m_device;
    }

    manager::Image &getImageManager()
    {
        return m_graphicsManagers.imageManager;
    }

    const manager::Image &getImageManager() const
    {
        return m_graphicsManagers.imageManager;
    }

    common::EventBus &getEventBus()
    {
        return m_eventBus;
    }

    job::TaskManager &getManager()
    {
        return m_taskManager;
    }

    ManagerCommandBufferWrapper getManagerCommandBuffer()
    {
        assert(m_commandBufferManager);

        return ManagerCommandBufferWrapper{.m_device = *m_device, .m_manager = *m_commandBufferManager};
    }

    ManagerRenderResource &getManagerRenderResource()
    {
        assert(m_renderResourceManager && "Manager not properly created");

        return *m_renderResourceManager;
    }

    manager::GraphicsContainer &getGraphicsManagers()
    {
        return m_graphicsManagers;
    }
    manager::DescriptorPool &getDescriptorPoolManager()
    {
        return *m_graphicsManagers.descriptorPoolManager;
    }

    manager::Shader &getShaderManager()
    {
        return *m_graphicsManagers.shaderManager;
    }

    manager::Pipeline &getPipelineManager()
    {
        return *m_graphicsManagers.pipelineManager;
    }

    manager::Semaphore &getSemaphoreManager()
    {
        return *m_graphicsManagers.semaphoreManager;
    }

    manager::Fence &getFenceManager()
    {
        return *m_graphicsManagers.fenceManager;
    }

    job::TransferWorker &getTransferWorker()
    {
        assert(m_transferWorker);

        return *m_transferWorker;
    }

    SwapChainSupportDetails getSwapchainSupportDetails();

    Handle &getDeviceID()
    {
        return m_deviceID;
    }

    RenderingSurface &getRenderingSurface()
    {
        return m_surface;
    }

    job::TaskManager &getTaskManager()
    {
        return m_taskManager;
    }
    const job::TaskManager &getTaskManager() const
    {
        return m_taskManager;
    }

    const uint64_t &getCurrentFrameIndex() const
    {
        return m_frameCounter;
    }

    const FrameInFlightTracking &getFrameInFlightTracking() const
    {
        return m_frameInFlightTrackingInfo;
    }

    void registerService(service::Service service, const uint8_t &numFramesInFlight);

  private:
    bool m_ownsResources = false;
    uint64_t m_frameCounter = 0;
    FrameInFlightTracking m_frameInFlightTrackingInfo;
    Handle m_deviceID;
    RenderingSurface m_surface;
    std::shared_ptr<StarDevice> m_device;
    common::EventBus m_eventBus;
    job::TaskManager m_taskManager;
    manager::GraphicsContainer m_graphicsManagers;
    std::unique_ptr<manager::ManagerCommandBuffer> m_commandBufferManager;
    std::shared_ptr<job::TransferWorker> m_transferWorker;
    std::unique_ptr<ManagerRenderResource> m_renderResourceManager;
    std::vector<service::Service> m_services;

    std::shared_ptr<job::TransferWorker> CreateTransferWorker(StarDevice &device,
                                                              const size_t &targetNumQueuesToUse = 2);

    void handleCompleteMessages(const uint8_t maxMessageCounter = 0);

    void processCompleteMessage(job::complete_tasks::CompleteTask completeTask);

    void setAllServiceParameters();

    void initWorkers(const uint8_t &numFramesInFlight);

    void shutdownServices();

    void logInit(const uint8_t &numFramesInFlight) const;

    void initServices(const uint8_t &numFramesInFlight);

    void broadcastFrameStart(const uint8_t &frameInFlightIndex);
};
} // namespace star::core::device