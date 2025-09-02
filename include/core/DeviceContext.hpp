#pragma once

#include "ManagerRenderResource.hpp"
#include "RenderingSurface.hpp"
#include "SwapChainSupportDetails.hpp"
#include "TaskManager.hpp"
#include "device/DeviceID.hpp"
#include "device/StarDevice.hpp"
#include "device/managers/ManagerCommandBuffer.hpp"
#include "tasks/TaskFactory.hpp"


#include "ConfigFile.hpp"

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

    DeviceContext(const DeviceID &deviceID, const uint8_t &numFramesInFlight, RenderingInstance &instance,
                  std::set<Rendering_Features> requiredFeatures, StarWindow &window,
                  const std::set<Rendering_Device_Features> &requiredRenderingDeviceFeatures);

    ~DeviceContext()
    {
        if (m_commandBufferManager)
            m_commandBufferManager->cleanup(*m_device);
    };

    DeviceContext(DeviceContext &&other) = default;
    DeviceContext &operator=(DeviceContext &&other)
    {
        m_surface = std::move(other.m_surface);
        m_device = std::move(other.m_device);
        m_manager = std::move(other.m_manager);
        m_commandBufferManager = std::move(other.m_commandBufferManager);

        return *this;
    };
    DeviceContext(const DeviceContext &) = delete;
    DeviceContext &operator=(const DeviceContext &) = delete;

    inline StarDevice &getDevice()
    {
        return *m_device;
    }

    job::TaskManager &getManager()
    {
        return m_manager;
    }

    std::shared_ptr<RenderingSurface> getSurface()
    {
        return m_surface;
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

  private:
    DeviceID m_deviceID;
    std::shared_ptr<RenderingSurface> m_surface = nullptr;
    std::shared_ptr<StarDevice> m_device;
    job::TaskManager m_manager;
    std::unique_ptr<managers::ManagerCommandBuffer> m_commandBufferManager = nullptr;
    std::unique_ptr<job::TransferWorker> m_transferWorker = nullptr;
    std::unique_ptr<ManagerRenderResource> m_renderResourceManager = nullptr;

    std::unique_ptr<job::TransferWorker> CreateTransferWorker(StarDevice &device);
};
} // namespace star::core::device