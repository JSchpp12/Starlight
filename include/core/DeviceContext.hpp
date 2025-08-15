#pragma once

#include "RenderingSurface.hpp"
#include "StarDevice.hpp"
#include "SwapChainSupportDetails.hpp"
#include "TaskManager.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>

namespace star::core
{
class DeviceContext
{
  public:
    DeviceContext(RenderingInstance &instance, std::set<Rendering_Features> requiredFeatures, StarWindow &window)
        : m_surface(std::make_shared<RenderingSurface>(instance, window)),
          m_device(StarDevice(window, *m_surface, instance, requiredFeatures)) {};

    ~DeviceContext() = default;

    DeviceContext(DeviceContext &&other) = default;
    DeviceContext &operator=(DeviceContext &&other) = default;
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

    SwapChainSupportDetails getSwapchainSupportDetails();

  private:
    std::shared_ptr<RenderingSurface> m_surface = nullptr;
    StarDevice m_device;
    job::TaskManager m_manager;
};
} // namespace star::core