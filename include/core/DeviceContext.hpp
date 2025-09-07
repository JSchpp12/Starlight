#pragma once

#include "ManagerRenderResource.hpp"
#include "RenderingSurface.hpp"
#include "SwapChainSupportDetails.hpp"
#include "TaskManager.hpp"
#include "device/DeviceID.hpp"
#include "device/StarDevice.hpp"
#include "device/managers/ManagerCommandBuffer.hpp"
#include "device/managers/ManagerShader.hpp"
#include "tasks/TaskFactory.hpp"

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
        Handle submit(device::managers::ManagerCommandBuffer::Request request)
        {
            return m_manager.submit(m_device, request);
        }

        vk::Semaphore update(const int &frameIndexToBeDrawn)
        {
            return m_manager.update(m_device, frameIndexToBeDrawn);
        }

        StarDevice &m_device;
        managers::ManagerCommandBuffer &m_manager;
    };

    struct ManagerShaderWrapper
    {
        Handle submit(StarShader shader)
        {
            return shaderManager.submit(taskManager, std::move(shader));
        }

        manager::Shader::Record &get(const Handle &handle)
        {
            return shaderManager.get(handle);
        }

        bool isReady(const Handle &handle)
        {
            return shaderManager.isReady(handle);
        }

        job::TaskManager &taskManager;
        manager::Shader &shaderManager;
    };

    DeviceContext(const uint8_t &numFramesInFlight, const DeviceID &deviceID, RenderingInstance &instance,
                  std::set<Rendering_Features> requiredFeatures, StarWindow &window,
                  const std::set<Rendering_Device_Features> &requiredRenderingDeviceFeatures);

    ~DeviceContext();
    DeviceContext(DeviceContext &&other)
        : m_deviceID(std::move(other.m_deviceID)), m_surface(std::move(other.m_surface)),
          m_device(std::move(other.m_device)), m_taskManager(std::move(other.m_taskManager)),
          m_commandBufferManager(std::move(other.m_commandBufferManager)),
          m_transferWorker(std::move(other.m_transferWorker))
    {
        other.m_ownsWorkers = false;
    };
    DeviceContext &operator=(DeviceContext &&other)
    {
        if (this != &other)
        {
            m_deviceID = std::move(other.m_deviceID);
            m_surface = std::move(other.m_surface);
            m_device = std::move(other.m_device);
            m_taskManager = std::move(other.m_taskManager);
            m_commandBufferManager = std::move(other.m_commandBufferManager);
            m_transferWorker = std::move(other.m_transferWorker);
            m_ownsWorkers = true;

            other.m_ownsWorkers = false;
        }

        return *this;
    };
    DeviceContext(const DeviceContext &) = delete;
    DeviceContext &operator=(const DeviceContext &) = delete;

    void prepareForNextFrame();

    inline StarDevice &getDevice()
    {
        return *m_device;
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
        return *m_renderResourceManager;
    }

    ManagerShaderWrapper getShaderManager()
    {
        return ManagerShaderWrapper{.taskManager = m_taskManager, .shaderManager = m_shaderManager};
    }

    job::TransferWorker &getTransferWorker()
    {
        assert(m_transferWorker);

        return *m_transferWorker;
    }

    SwapChainSupportDetails getSwapchainSupportDetails();

    DeviceID getDeviceID()
    {
        return m_deviceID;
    }

    RenderingSurface &getRenderingSurface(){
        return m_surface;
    }

  private:
    bool m_ownsWorkers = true;
    // Resolution of the final image to be produced -- usually set from the window
    uint64_t m_frameCounter = 0;
    DeviceID m_deviceID;
    RenderingSurface m_surface;
    std::shared_ptr<StarDevice> m_device;
    job::TaskManager m_taskManager;
    manager::Shader m_shaderManager;
    std::unique_ptr<managers::ManagerCommandBuffer> m_commandBufferManager;
    std::shared_ptr<job::TransferWorker> m_transferWorker;
    std::unique_ptr<ManagerRenderResource> m_renderResourceManager;

    std::shared_ptr<job::TransferWorker> CreateTransferWorker(StarDevice &device);

    void handleCompleteMessages(const uint8_t maxMessageCounter = 0);

    void processCompleteMessage(job::complete_tasks::CompleteTask<> completeTask);
};
} // namespace star::core::device