#pragma once

#include "RenderingInstance.hpp"
#include "StarWindow.hpp"


#include <vulkan/vulkan.hpp>

namespace star::core
{
class RenderingSurface
{
  public:
    RenderingSurface(RenderingInstance &instance, StarWindow &window);
    ~RenderingSurface() {

    };

    vk::SurfaceKHR &getSurface()
    {
        return m_surface.get();
    }

  private:
    vk::UniqueSurfaceKHR m_surface;

    static vk::UniqueSurfaceKHR CreateSurface(RenderingInstance &instance, StarWindow &window);
};
} // namespace star::core