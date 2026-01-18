#pragma once

#include "Allocator.hpp"
#include "Enums.hpp"
#include "RenderingInstance.hpp"
#include "SwapChainSupportDetails.hpp"
#include "wrappers/graphics/QueueFamilyIndices.hpp"

#include <star_common/IRenderDevice.hpp>

#include <vulkan/vulkan.hpp>

#include <iostream>
#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

namespace star::core::device
{

class StarDevice : public star::common::IRenderDevice
{
  public:
    StarDevice() = default;

    ~StarDevice();

    // Not copyable
    StarDevice(const StarDevice &) = delete;
    StarDevice &operator=(const StarDevice &) = delete;
    StarDevice(StarDevice &&other) noexcept;
    StarDevice &operator=(StarDevice &&other) noexcept;

    StarDevice(core::RenderingInstance &renderingInstance, std::set<star::Rendering_Features> requiredFeatures,
               const std::set<star::Rendering_Device_Features> &requiredRenderingDeviceFeatures,
               const std::vector<const char *> additionalRequiredPhysicalDeviceExtensions,
               vk::SurfaceKHR *optionalRenderingSurface = nullptr);

    /// <summary>
    /// Check the hardware to make sure that the supplied formats are compatible with the current system.
    /// </summary>
    /// <param name="candidates">List of possible formats to check</param>
    /// <param name="tiling"></param>
    /// <param name="features"></param>
    /// <returns></returns>
    bool findSupportedFormat(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling,
                             vk::FormatFeatureFlags features, vk::Format &selectedFormat) const;
#pragma region getters
    vk::PhysicalDevice getPhysicalDevice()
    {
        return this->physicalDevice;
    }
    inline vk::Device &getVulkanDevice()
    {
        return this->vulkanDevice;
    }
    void *getNativeDevice() override
    {
        return static_cast<void *>(vulkanDevice);
    }
    Allocator &getAllocator()
    {
        return allocator;
    }

    core::SwapChainSupportDetails getSwapchainSupport(vk::SurfaceKHR surface)
    {
        return QuerySwapchainSupport(this->physicalDevice, surface);
    }
#pragma endregion

#pragma region helperFunctions
    /// <summary>
    /// Create a buffer with the given arguments
    /// </summary>
    void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usageFlags, vk::MemoryPropertyFlags properties,
                      vk::Buffer &buffer, vk::DeviceMemory &bufferMemory);

    void createImageWithInfo(const vk::ImageCreateInfo &imageInfo, vk::MemoryPropertyFlags properties, vk::Image &image,
                             vk::DeviceMemory &imageMemory);

    /// <summary>
    /// Query the GPU for the proper memory type that matches properties defined in passed arguments.
    /// </summary>
    /// <param name="typeFilter">Which bit field of memory types that are suitable</param>
    /// <param name="properties"></param>
    /// <returns></returns>
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags propertyFlags);

    bool verifyImageCreate(vk::ImageCreateInfo imageInfo);

    QueueFamilyIndices getQueueInfo(); 
#pragma endregion

  protected:
    star::Allocator allocator;
    vk::Device vulkanDevice = VK_NULL_HANDLE;
    vk::PhysicalDevice physicalDevice = VK_NULL_HANDLE;
    vk::SurfaceKHR m_optionalRenderingSurface = VK_NULL_HANDLE; 

    // Pick a proper physical GPU that matches the required extensions
    void pickPhysicalDevice(core::RenderingInstance &instance, const vk::PhysicalDeviceFeatures &requiredDeviceFeatures,
                            const std::vector<const char *> &requiredDeviceExtensions,
                            vk::SurfaceKHR *optionalRenderingSurface);
    // Create a logical device to communicate with the physical device
    void createLogicalDevice(core::RenderingInstance &instance,
                             const vk::PhysicalDeviceFeatures &requiredDeviceFeatures,
                             const std::vector<const char *> &requiredDeviceExtensions,
                             const std::set<Rendering_Device_Features> &deviceFeatures,
                             vk::SurfaceKHR *optionalRenderingSurface);

    void createAllocator(core::RenderingInstance &instance);

    std::vector<const char *> getDefaultPhysicalDeviceExtensions() const
    {
        return {VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
                VK_KHR_BIND_MEMORY_2_EXTENSION_NAME,
                VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
                VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
                VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME,
                VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
                VK_EXT_DYNAMIC_RENDERING_UNUSED_ATTACHMENTS_EXTENSION_NAME};
    }

    /* Helper Functions */

    // Helper function to test each potential GPU device
    static bool IsDeviceSuitable(const std::vector<const char *> &requiredDeviceExtensions,
                                 const vk::PhysicalDeviceFeatures &requiredDeviceFeatures,
                                 const vk::PhysicalDevice &device, vk::SurfaceKHR *optionalRenderingSurface = nullptr);

    /// <summary>
    /// Find what queues are available for the device
    /// Queues support different types of commands such as : processing compute commands or memory transfer commands
    /// </summary>
    static QueueFamilyIndices FindQueueFamilies(const vk::PhysicalDevice &device,
                                                 vk::SurfaceKHR *optionalRenderingSurface);

    /// <summary>
    /// Check if the given device supports required extensions.
    /// </summary>
    static bool CheckDeviceExtensionSupport(const vk::PhysicalDevice &device,
                                            const std::vector<const char *> &requiredDeviceExtensions);

    /// <summary>
    /// Request specific details about swap chain support for a given device
    /// </summary>
    static core::SwapChainSupportDetails QuerySwapchainSupport(const vk::PhysicalDevice &device,
                                                               vk::SurfaceKHR surface);

  private:
    static bool DoesDeviceSupportPresentation(vk::PhysicalDevice device, const vk::SurfaceKHR &surface);
};
} // namespace star::core::device