#pragma once

#include "Allocator.hpp"
#include "Enums.hpp"
#include "RenderingInstance.hpp"
#include "StarCommandBuffer.hpp"
#include "StarCommandPool.hpp"
#include "StarQueueFamily.hpp"
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
    class QueueOwnershipTracker
    {
      public:
        QueueOwnershipTracker(std::vector<StarQueueFamily> regQueueFamilies)
            : regQueueFamilies(regQueueFamilies),
              isQueueAvailable(std::vector<std::vector<bool>>(regQueueFamilies.size()))
        {
            for (size_t i = 0; i < this->regQueueFamilies.size(); i++)
            {
                this->isQueueAvailable.at(i) = std::vector<bool>(this->regQueueFamilies.at(i).getQueueCount(), true);
            }
        }

        std::optional<StarQueue> giveMeQueueWithProperties(const vk::QueueFlags &capabilities,
                                                           const bool &presentationSupport = false)
        {
            for (size_t i = 0; i < this->regQueueFamilies.size(); i++)
            {
                if (this->regQueueFamilies.at(i).doesSupport(capabilities, presentationSupport))
                {
                    for (size_t j = 0; j < this->isQueueAvailable.at(i).size(); j++)
                    {
                        if (this->isQueueAvailable.at(i).at(j))
                        {
                            this->isQueueAvailable.at(i).at(j) = false;
                            return std::make_optional<StarQueue>(this->regQueueFamilies.at(i).getQueues().at(j));
                        }
                    }
                }
            }

            return std::nullopt;
        }

        std::optional<StarQueue> giveMeQueueWithProperties(const vk::QueueFlags &capabilities,
                                                           const bool &presentationSupport, const uint32_t &familyIndex)
        {
            for (size_t i = 0; i < this->regQueueFamilies.size(); i++)
            {
                if (this->regQueueFamilies.at(i).getQueueFamilyIndex() == familyIndex &&
                    this->regQueueFamilies.at(i).doesSupport(capabilities, presentationSupport))
                {
                    for (size_t j = 0; j < this->isQueueAvailable.at(i).size(); j++)
                    {
                        if (this->isQueueAvailable.at(i).at(j))
                        {
                            this->isQueueAvailable.at(i).at(j) = false;
                            return std::make_optional(this->regQueueFamilies.at(i).getQueues().at(j));
                        }
                    }
                }
            }

            return std::nullopt;
        }

        std::vector<uint32_t> getQueueFamiliesWhichSupport(const vk::QueueFlags &capabilities,
                                                           const bool &presentationSupport = false)
        {
            std::vector<uint32_t> indices = std::vector<uint32_t>();

            for (size_t i = 0; i < this->regQueueFamilies.size(); i++)
            {
                if (this->regQueueFamilies.at(i).doesSupport(capabilities, presentationSupport))
                {
                    indices.push_back(this->regQueueFamilies.at(i).getQueueFamilyIndex());
                }
            }

            return indices;
        }

        std::vector<uint32_t> getAllQueueFamilyIndices() const
        {
            std::vector<uint32_t> indices = std::vector<uint32_t>();
            for (size_t i = 0; i < this->regQueueFamilies.size(); i++)
            {
                indices.emplace_back(this->regQueueFamilies.at(i).getQueueFamilyIndex());
            }

            return indices;
        }

      private:
        std::vector<StarQueueFamily> regQueueFamilies = std::vector<StarQueueFamily>();
        std::vector<std::vector<bool>> isQueueAvailable = std::vector<std::vector<bool>>();
    };
    StarDevice() = default;

    StarDevice(core::RenderingInstance &renderingInstance, std::set<star::Rendering_Features> requiredFeatures,
               const std::set<star::Rendering_Device_Features> &requiredRenderingDeviceFeatures,
               const std::vector<const char *> additionalRequiredPhysicalDeviceExtensions,
               vk::SurfaceKHR *optionalRenderingSurface = nullptr);

    ~StarDevice();

    // Not copyable
    StarDevice(const StarDevice &) = delete;
    StarDevice &operator=(const StarDevice &) = delete;

    StarDevice(StarDevice &&other)
        : vulkanDevice(other.vulkanDevice), allocator(std::move(other.allocator)), physicalDevice(other.physicalDevice),
          extraFamilies(std::move(other.extraFamilies)), currentDeviceQueues(std::move(other.currentDeviceQueues)),
          defaultQueue(std::move(other.defaultQueue)), dedicatedComputeQueue(std::move(other.dedicatedComputeQueue)),
          dedicatedTransferQueue(std::move(other.dedicatedTransferQueue)), defaultCommandPool(other.defaultCommandPool),
          transferCommandPool(other.transferCommandPool), computeCommandPool(other.computeCommandPool)
    {
        other.vulkanDevice = VK_NULL_HANDLE;
    }

    StarDevice &operator=(StarDevice &&other)
    {
        if (this != &other)
        {
            vulkanDevice = other.vulkanDevice;
            allocator = std::move(other.allocator);
            physicalDevice = other.physicalDevice;
            extraFamilies = std::move(other.extraFamilies);
            currentDeviceQueues = std::move(other.currentDeviceQueues);
            defaultQueue = std::move(other.defaultQueue);
            dedicatedComputeQueue = std::move(other.dedicatedComputeQueue);
            dedicatedTransferQueue = std::move(other.dedicatedTransferQueue);
            defaultCommandPool = std::move(other.defaultCommandPool);
            transferCommandPool = std::move(other.transferCommandPool);
            computeCommandPool = std::move(other.computeCommandPool);

            other.vulkanDevice = VK_NULL_HANDLE;
        }

        return *this;
    }

    /// <summary>
    /// Check the hardware to make sure that the supplied formats are compatible with the current system.
    /// </summary>
    /// <param name="candidates">List of possible formats to check</param>
    /// <param name="tiling"></param>
    /// <param name="features"></param>
    /// <returns></returns>
    bool findSupportedFormat(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling,
                             vk::FormatFeatureFlags features, vk::Format &selectedFormat) const;

    std::shared_ptr<StarCommandPool> getCommandPool(const star::Queue_Type &type);

    StarQueue &getDefaultQueue(const star::Queue_Type &type);

    QueueOwnershipTracker &getQueueOwnershipTracker()
    {
        assert(this->currentDeviceQueues != nullptr);

        return *this->currentDeviceQueues;
    }

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
        assert(this->allocator &&
               "Default allocator should have been created during startup. Something has gone wrong.");
        return *this->allocator;
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

    /// <summary>
    /// Helper function to execute single time use command buffers
    /// </summary>
    /// <param name="useTransferPool">Should command be submitted to the transfer command pool. Will be submitted to the
    /// graphics pool otherwise.</param> <returns></returns>
    std::unique_ptr<StarCommandBuffer> beginSingleTimeCommands();

    /// <summary>
    /// Helper function to end execution of single time use command buffer
    /// </summary>
    /// <param name="commandBuffer"></param>
    /// <param name="useTransferPool">Was command buffer submitted to the transfer pool. Assumed graphics pool
    /// otherwise.</param>
    void endSingleTimeCommands(std::unique_ptr<StarCommandBuffer> commandBuffer,
                               vk::Semaphore *signalFinishSemaphore = nullptr);

    void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size,
                    const vk::DeviceSize dstOffset = 0);

    /// <summary>
    /// Copy a buffer to an image.
    /// </summary>
    /// <param name="buffer"></param>
    /// <param name="image"></param>
    /// <param name="width"></param>
    /// <param name="height"></param>
    void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);

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

#pragma endregion

  protected:
    vk::Device vulkanDevice;
    std::unique_ptr<star::Allocator> allocator;
    vk::PhysicalDevice physicalDevice = VK_NULL_HANDLE;

    std::vector<std::unique_ptr<StarQueueFamily>> extraFamilies = std::vector<std::unique_ptr<StarQueueFamily>>();
    std::unique_ptr<QueueOwnershipTracker> currentDeviceQueues = std::unique_ptr<QueueOwnershipTracker>();

    std::unique_ptr<StarQueue> defaultQueue = nullptr;
    std::unique_ptr<StarQueue> dedicatedComputeQueue = nullptr;
    std::unique_ptr<StarQueue> dedicatedTransferQueue = nullptr;

    std::shared_ptr<StarCommandPool> defaultCommandPool = nullptr;
    std::shared_ptr<StarCommandPool> transferCommandPool = nullptr;
    std::shared_ptr<StarCommandPool> computeCommandPool = nullptr;

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

    std::vector<const char *> getDefaultPhysicalDeviceExtensions()
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
    static QueueFamilyIndicies FindQueueFamilies(const vk::PhysicalDevice &device,
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