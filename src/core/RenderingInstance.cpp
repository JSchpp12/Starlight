#include "RenderingInstance.hpp"

#include "CastHelpers.hpp"

#include <unordered_set>

star::core::RenderingInstance::RenderingInstance(const std::string &applicationName)
{
    if (m_enableValidationLayers && !DoesSystemSupportValidationLayers(m_validationLayers))
    {
        throw std::runtime_error("Validation layers were requested but are not available on the system. Ensure PATH "
                                 "and vulkan dependencies are properly set");
    }

    if (!DoesSystemSupportDisplayExtensions(getRequiredDisplayExtensions())){
        throw std::runtime_error("System does not support the required extensions for display");
    }

    m_instance = createInstance(applicationName);
}

star::core::RenderingInstance::~RenderingInstance()
{
    if (m_instance)
        m_instance.destroy(); 
}

vk::Instance star::core::RenderingInstance::createInstance(const std::string &applicationName)
{
    std::vector<const char *> extensions = getRequiredDisplayExtensions();
    for (const auto &extension : m_platformInstanceRequiredExtensions)
    {
        extensions.push_back(extension);
    }

    if (m_enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    uint32_t extensionCount = 0;
    CastHelpers::SafeCast<size_t, uint32_t>(extensions.size(), extensionCount);

    uint32_t validationLayerCount = 0;
    CastHelpers::SafeCast<size_t, uint32_t>(m_validationLayers.size(), validationLayerCount);

    auto appInfo = vk::ApplicationInfo()
                       .setPApplicationName(applicationName.c_str())
                       .setApplicationVersion(vk::makeApiVersion(0, 1, 0, 0))
                       .setPEngineName("Starlight")
                       .setEngineVersion(vk::makeApiVersion(0, 1, 0, 0))
                       .setApiVersion(vk::ApiVersion13);

    return vk::createInstance(
        vk::InstanceCreateInfo()
            .setEnabledExtensionCount(extensionCount)
            .setPpEnabledExtensionNames(extensions.data())
            .setEnabledLayerCount(validationLayerCount)
            .setPpEnabledLayerNames(m_validationLayers.data())
            .setPApplicationInfo(&appInfo)
            .setFlags(m_isMac ? vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR : vk::InstanceCreateFlags()));
}

#ifdef ENABLE_PRESENTATION
#include "GLFW/glfw3.h"
std::vector<const char *> star::core::RenderingInstance::getRequiredDisplayExtensions() const
{
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    return std::vector<const char *>(glfwExtensions, glfwExtensions);
}

#else
std::vector<const char *> star::core::RenderingInstance::getRequiredDisplayExtensions() const
{
    return std::vector<const char *>();
}
#endif

bool star::core::RenderingInstance::DoesSystemSupportValidationLayers(const std::vector<const char *> &validationLayers)
{
    bool doesSupport = true; 

    std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

    for (const char *layerName : validationLayers)
    {
        bool layerFound = false;

        for (const auto &layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            doesSupport = false; 
            std::cerr << "Missing Validation layer " << layerName << std::endl;
        }
    }

    return doesSupport; 
}

bool star::core::RenderingInstance::DoesSystemSupportDisplayExtensions(const std::vector<const char *> &requiredDisplayExtensions)
{
    bool doesSupport = true; 

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    std::unordered_set<std::string> available;
    for (const auto &extension : extensions)
    {
        available.insert(extension.extensionName);
    }

    for (const auto& required : requiredDisplayExtensions){
        if (available.find(required) == available.end()){
            std::cerr << "Missing required display extension: " << required << std::endl;
            doesSupport = false; 
        }
    }

    return doesSupport; 
}
