#pragma once

#include "ManagerRenderResource.hpp"
#include "RenderingSurface.hpp"
#include "SwapChainSupportDetails.hpp"
#include "TaskManager.hpp"
#include "core/device/FrameInFlightTracking.hpp"
#include "device/StarDevice.hpp"
#include "device/managers/GraphicsContainer.hpp"
#include "device/managers/ManagerCommandBuffer.hpp"
#include "device/managers/Pipeline.hpp"
#include "device/system/EventBus.hpp"
#include "tasks/TaskFactory.hpp"
#include "service/Service.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <vector>

namespace star::core::device
{
class DeviceContext
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
    template <typename TManager, typename TRequest, typename TRecord> struct TaskManagerWrapper
    {
        TaskManagerWrapper(TManager &manager, DeviceContext &context)
            : manager(manager), device(*context.m_device), taskManager(context.m_taskManager),
              eventBus(context.m_eventBus)
        {
        }

        Handle submit(TRequest request)
        {
            return manager.submit(device, std::move(request), taskManager, eventBus);
        }

        TRecord *get(const Handle &handle)
        {
            return manager.get(handle);
        }

        void deleteRecord(const Handle &handle)
        {
            manager.deleteRecord(handle);
        }

        TManager &manager;
        device::StarDevice &device;
        job::TaskManager &taskManager;
        system::EventBus &eventBus;
    };
    template <typename TManager, typename TRequest, typename TRecord> struct ManagerWrapper
    {
        ManagerWrapper(TManager &manager, DeviceContext &context) : manager(manager), device(*context.m_device)
        {
        }

        Handle submit(TRequest request)
        {
            return manager.submit(device, std::move(request));
        }

        TRecord *get(const Handle &handle)
        {
            return manager.get(handle);
        }

        void deleteRecord(const Handle &handle)
        {
            manager.deleteRecord(handle);
        }

        TManager &manager;
        device::StarDevice &device;
    };
    DeviceContext() = default;
    ~DeviceContext();
    DeviceContext(DeviceContext &&other)
        : m_frameInFlightTrackingInfo(std::move(other.m_frameInFlightTrackingInfo)),
          m_deviceID(std::move(other.m_deviceID)), m_surface(std::move(other.m_surface)),
          m_device(std::move(other.m_device)), m_eventBus(std::move(other.m_eventBus)),
          m_taskManager(std::move(other.m_taskManager)), m_graphicsManagers(std::move(other.m_graphicsManagers)),
          m_commandBufferManager(std::move(other.m_commandBufferManager)),
          m_transferWorker(std::move(other.m_transferWorker)),
          m_renderResourceManager(std::move(other.m_renderResourceManager)), m_services(std::move(other.m_services))
    {
        other.m_ownsResources = false;
    };
    DeviceContext &operator=(DeviceContext &&other)
    {
        if (this != &other)
        {
            m_frameInFlightTrackingInfo = std::move(other.m_frameInFlightTrackingInfo);
            m_frameCounter = std::move(other.m_frameCounter);
            m_deviceID = std::move(other.m_deviceID);
            m_surface = std::move(other.m_surface);
            m_device = std::move(other.m_device);
            m_eventBus = std::move(other.m_eventBus);
            m_taskManager = std::move(other.m_taskManager);
            m_commandBufferManager = std::move(other.m_commandBufferManager);
            m_graphicsManagers = std::move(other.m_graphicsManagers);
            m_transferWorker = std::move(other.m_transferWorker);
            m_renderResourceManager = std::move(other.m_renderResourceManager);
            m_services = std::move(other.m_services);
            if (other.m_ownsResources)
            {
                m_ownsResources = std::move(other.m_ownsResources);

                setAllServiceParameters();
            }

            other.m_ownsResources = false;
        }

        return *this;
    };
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

    core::device::system::EventBus &getEventBus()
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

    TaskManagerWrapper<manager::Shader, manager::ShaderRequest, manager::ShaderRecord> getShaderManager()
    {
        return TaskManagerWrapper<manager::Shader, manager::ShaderRequest, manager::ShaderRecord>{
            *m_graphicsManagers.shaderManager, *this};
    }

    TaskManagerWrapper<manager::Pipeline, manager::PipelineRequest, manager::PipelineRecord> getPipelineManager()
    {
        return TaskManagerWrapper<manager::Pipeline, manager::PipelineRequest, manager::PipelineRecord>(
            *m_graphicsManagers.pipelineManager, *this);
    }

    ManagerWrapper<manager::Semaphore &, manager::SemaphoreRequest, manager::SemaphoreRecord> getSemaphoreManager()
    {
        return ManagerWrapper<manager::Semaphore &, manager::SemaphoreRequest, manager::SemaphoreRecord>(
            *m_graphicsManagers.semaphoreManager, *this);
    }

    ManagerWrapper<manager::Fence &, manager::FenceRequest, manager::FenceRecord> getFenceManager()
    {
        return {*m_graphicsManagers.fenceManager, *this};
    }

    job::TransferWorker &getTransferWorker()
    {
        assert(m_transferWorker);

        return *m_transferWorker;
    }

    SwapChainSupportDetails getSwapchainSupportDetails();

    Handle getDeviceID()
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

    const FrameInFlightTracking &getFrameInFlightTracking(const uint8_t &frameInFlightIndex) const
    {
        return m_frameInFlightTrackingInfo[frameInFlightIndex];
    }

    void registerService(service::Service service, const uint8_t &numFramesInFlight);
  private:
    bool m_ownsResources = false;
    uint64_t m_frameCounter = 0;
    std::vector<FrameInFlightTracking> m_frameInFlightTrackingInfo;
    Handle m_deviceID;
    RenderingSurface m_surface;
    std::shared_ptr<StarDevice> m_device;
    system::EventBus m_eventBus;
    job::TaskManager m_taskManager;
    manager::GraphicsContainer m_graphicsManagers;
    std::unique_ptr<manager::ManagerCommandBuffer> m_commandBufferManager;
    std::shared_ptr<job::TransferWorker> m_transferWorker;
    std::unique_ptr<ManagerRenderResource> m_renderResourceManager;
    std::vector<service::Service> m_services;

    std::shared_ptr<job::TransferWorker> CreateTransferWorker(StarDevice &device);

    void handleCompleteMessages(const uint8_t maxMessageCounter = 0);

    void processCompleteMessage(job::complete_tasks::CompleteTask completeTask);

    void setAllServiceParameters();

    void initWorkers(const uint8_t &numFramesInFlight);

    void shutdownServices();

    void logInit(const uint8_t &numFramesInFlight) const;
    
    void initServices(const uint8_t &numFramesInFlight);
};
} // namespace star::core::device