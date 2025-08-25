#pragma once

#include "devices/managers/ManagerCommandBuffer.hpp"
#include "RenderingSurface.hpp"
#include "devices/StarDevice.hpp"
#include "SwapChainSupportDetails.hpp"
#include "TaskManager.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>

namespace star::core::devices
{
class DeviceContext
{
  public:
    struct ManagerCommandBufferWrapper
    {
        Handle submit(devices::managers::ManagerCommandBuffer::Request request)
        {
            return m_manager.submit(m_device, request);
        }

        vk::Semaphore update(const int &frameIndexToBeDrawn){
            return m_manager.update(m_device, frameIndexToBeDrawn); 
        }

        StarDevice &m_device;
        managers::ManagerCommandBuffer &m_manager;
    };

    DeviceContext(const uint8_t &numFramesInFlight, RenderingInstance &instance,
                  std::set<Rendering_Features> requiredFeatures, StarWindow &window)
        : m_surface(std::make_shared<RenderingSurface>(instance, window)),
          m_device(StarDevice(window, *m_surface, instance, requiredFeatures)),
          m_commandBufferManager(std::make_unique<managers::ManagerCommandBuffer>(m_device, numFramesInFlight)) {};

    ~DeviceContext()
    {
        if (m_commandBufferManager)
            m_commandBufferManager->cleanup(m_device);
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
        return m_device;
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

        return ManagerCommandBufferWrapper{.m_device = m_device, .m_manager = *m_commandBufferManager};
    }

    SwapChainSupportDetails getSwapchainSupportDetails();

  private:
    std::shared_ptr<RenderingSurface> m_surface = nullptr;
    StarDevice m_device;
    job::TaskManager m_manager;
    std::unique_ptr<managers::ManagerCommandBuffer> m_commandBufferManager = nullptr;
};
} // namespace star::core