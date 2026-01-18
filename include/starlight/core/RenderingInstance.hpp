#pragma once

#include <vulkan/vulkan.hpp>

#include <string>
#include <vector>

namespace star::core
{
class RenderingInstance
{
  public:
    RenderingInstance(const std::string &applicationName, std::vector<const char *> &extensions);
    ~RenderingInstance();

    RenderingInstance(const RenderingInstance &) = delete;
    RenderingInstance(RenderingInstance &&other) : m_instance(other.m_instance)
    {
        other.m_instance = VK_NULL_HANDLE;
    }

    RenderingInstance &operator=(RenderingInstance &&other)
    {
        m_instance = other.m_instance;
        other.m_instance = VK_NULL_HANDLE;
        return *this;
    }
    RenderingInstance &operator=(RenderingInstance &other) = delete;

    vk::Instance &getVulkanInstance()
    {
        return m_instance;
    }

    bool getEnableValidationLayers() const
    {
        return m_enableValidationLayers;
    }

    const std::vector<const char *> getValidationLayerNames() const
    {
        return m_validationLayers;
    }

    std::vector<const char *> getRequiredDisplayExtensions() const;

  private:
    vk::Instance m_instance;

#ifdef NDEBUG
    const bool m_enableValidationLayers = false;
    const std::vector<const char *> m_validationLayers = {};
#else
    const bool m_enableValidationLayers = true;
    const std::vector<const char *> m_validationLayers = {"VK_LAYER_KHRONOS_validation"};
#endif

#if __APPLE__
    bool m_isMac = true;
    std::vector<const char *> m_platformInstanceRequiredExtensions = {
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, "VK_KHR_portability_enumeration"};
#else
    bool m_isMac = false;
    std::vector<const char *> m_platformInstanceRequiredExtensions = {};
#endif

    vk::Instance createInstance(const std::string &applicationName, std::vector<const char *> &extensions);

    static bool DoesSystemSupportValidationLayers(const std::vector<const char *> &validationLayers);

    static bool DoesSystemSupportDisplayExtensions(const std::vector<const char *> &requiredDisplayExtensions);
};
} // namespace star::core