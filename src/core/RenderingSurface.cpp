#include "RenderingSurface.hpp"

star::core::RenderingSurface::RenderingSurface(RenderingInstance &instance, StarWindow &window)
    : m_surface(CreateSurface(instance, window))
{
}

vk::UniqueSurfaceKHR star::core::RenderingSurface::CreateSurface(RenderingInstance &instance, StarWindow &window)
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
