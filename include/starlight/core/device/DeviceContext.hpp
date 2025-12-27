#pragma once

#include "TaskManager.hpp"
#include "device/StarDevice.hpp"
#include "device/managers/GraphicsContainer.hpp"
#include "device/managers/ManagerCommandBuffer.hpp"
#include "device/managers/Pipeline.hpp"
#include "service/Service.hpp"
#include "tasks/TaskFactory.hpp"

#include <star_common/FrameTracker.hpp>
#include <star_common/IDeviceContext.hpp>

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

        vk::Semaphore update(const common::FrameTracker &frameTracker)
        {
            return m_manager.update(m_device, frameTracker);
        }

        StarDevice &m_device;
        manager::ManagerCommandBuffer &m_manager;
    };
    DeviceContext() = default;
    explicit DeviceContext(StarDevice device) : m_device(std::move(device)){};

    virtual ~DeviceContext();
    DeviceContext(DeviceContext &&other);
    DeviceContext &operator=(DeviceContext &&other);
    DeviceContext(const DeviceContext &) = delete;
    DeviceContext &operator=(const DeviceContext &) = delete;

    void init(const Handle &deviceID, common::FrameTracker::Setup setup, vk::Extent2D engineResolution);

    void waitIdle();

    void prepareForNextFrame();

    StarDevice &getDevice()
    {
        return m_device;
    }
    const StarDevice &getDevice() const
    {
        return m_device;
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

        return ManagerCommandBufferWrapper{.m_device = m_device, .m_manager = *m_commandBufferManager};
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

    Handle &getDeviceID()
    {
        return m_deviceID;
    }

    job::TaskManager &getTaskManager()
    {
        return m_taskManager;
    }
    const job::TaskManager &getTaskManager() const
    {
        return m_taskManager;
    }

    const common::FrameTracker &getFrameTracker() const
    {
        return m_flightTracker;
    }
    vk::Extent2D &getEngineResolution()
    {
        return m_engineResolution;
    }
    const vk::Extent2D &getEngineResolution() const
    {
        return m_engineResolution;
    }

    void cleanupRender(); 

    void registerService(service::Service service);

  private:
    StarDevice m_device;
    common::FrameTracker m_flightTracker;
    bool m_ownsResources = false;
    Handle m_deviceID;
    common::EventBus m_eventBus;
    job::TaskManager m_taskManager;
    manager::GraphicsContainer m_graphicsManagers;
    std::unique_ptr<manager::ManagerCommandBuffer> m_commandBufferManager;
    std::shared_ptr<job::TransferWorker> m_transferWorker;
    std::unique_ptr<ManagerRenderResource> m_renderResourceManager;
    std::vector<service::Service> m_services;
    vk::Extent2D m_engineResolution;

    std::shared_ptr<job::TransferWorker> CreateTransferWorker(StarDevice &device,
                                                              const size_t &targetNumQueuesToUse = 2);

    void handleCompleteMessages(const uint8_t maxMessageCounter = 0);

    void processCompleteMessage(job::complete_tasks::CompleteTask completeTask);

    void setAllServiceParameters();

    void initWorkers(const uint8_t &numFramesInFlight);

    void shutdownServices();

    void logInit(const uint8_t &numFramesInFlight) const;

    void initServices(const uint8_t &numFramesInFlight);

    void broadcastFrameStart();

    void broadcastFramePrepToService();
};
} // namespace star::core::device