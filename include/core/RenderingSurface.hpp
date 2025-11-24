#pragma once

#include "RenderingInstance.hpp"
#include "StarWindow.hpp"

#include <vulkan/vulkan.hpp>

namespace star::core
{
class RenderingSurface
{
  public:
    RenderingSurface() = default;

    RenderingSurface(const RenderingSurface &) = delete;
    RenderingSurface &operator=(const RenderingSurface &) = delete;
    RenderingSurface(RenderingSurface &&other) = default;
    RenderingSurface &operator=(RenderingSurface &&other) = default;
    ~RenderingSurface() = default;

    void init(RenderingInstance &instance, StarWindow &window);

    vk::SurfaceKHR &getVulkanSurface()
    {
        return m_surface.get();
    }
    const vk::SurfaceKHR &getVulkanSurface() const
    {
        return m_surface.get();
    }
    vk::Extent2D &getResolution()
    {
        return m_resolution;
    }
    const vk::Extent2D &getResolution() const
    {
        return m_resolution;
    }

  private:
    vk::Extent2D m_resolution;
    vk::UniqueSurfaceKHR m_surface;

    static vk::UniqueSurfaceKHR CreateSurface(RenderingInstance &instance, StarWindow &window);
};
} // namespace star::core