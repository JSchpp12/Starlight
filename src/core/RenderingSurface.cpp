#include "RenderingSurface.hpp"

namespace star::core
{
vk::UniqueSurfaceKHR RenderingSurface::CreateSurface(RenderingInstance &instance, StarWindow &window)
{
    VkSurfaceKHR surfaceTmp = VkSurfaceKHR();

    auto createResult =
        glfwCreateWindowSurface(instance.getVulkanInstance(), window.getGLFWwindow(), nullptr, &surfaceTmp);
    if (createResult != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface");
    }
    return vk::UniqueSurfaceKHR(surfaceTmp, instance.getVulkanInstance());
}

void RenderingSurface::init(RenderingInstance &instance, StarWindow &window)
{
    m_resolution = window.getExtent();
    m_surface = CreateSurface(instance, window);
}
} // namespace star::core
