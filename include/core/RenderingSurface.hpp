#pragma once

#include "RenderingInstance.hpp"
#include "StarWindow.hpp"

#include <vulkan/vulkan.hpp>

namespace star::core
{
class RenderingSurface
{
  public:
    RenderingSurface(RenderingInstance &instance, StarWindow &window)
        : m_resolution(window.getExtent()), m_surface(CreateSurface(instance, window)) {};

    RenderingSurface(const RenderingSurface &) = delete;
    RenderingSurface &operator=(const RenderingSurface &) = delete;
    RenderingSurface(RenderingSurface &&other)
        : m_resolution(std::move(other.m_resolution)), m_surface(std::move(other.m_surface))
    {
    }
    RenderingSurface &operator=(RenderingSurface &&other)
    {
        if (this != &other)
        {
            m_resolution = std::move(other.m_resolution);
            m_surface = std::move(other.m_surface);
        }
        return *this;
    }

    vk::SurfaceKHR &getVulkanSurface()
    {
        return m_surface.get();
    }

    vk::Extent2D getResolution() const
    {
        return m_resolution;
    }

  private:
    vk::Extent2D m_resolution;
    vk::UniqueSurfaceKHR m_surface;

    static vk::UniqueSurfaceKHR CreateSurface(RenderingInstance &instance, StarWindow &window);
};
} // namespace star::core