#pragma once

#include "Allocator.hpp"
#include "CastHelpers.hpp"
#include "Enums.hpp"
#include "StarCommandBuffer.hpp"
#include "StarCommandPool.hpp"
#include "StarQueueFamily.hpp"
#include "StarWindow.hpp"

#include <vulkan/vulkan.hpp>

#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <unordered_set>
#include <vector>

namespace star
{
struct SwapChainSupportDetails
{
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

class QueueFamilyIndicies
{
  public:
    void registerFamily(const uint32_t &familyIndex, const vk::QueueFlags &queueSupport,
                        const vk::Bool32 &presentSupport, const uint32_t &familyQueueCount)
    {
        this->familyIndexQueueCount[familyIndex] = familyQueueCount;
        this->familyIndexQueueSupport[familyIndex] = queueSupport;
        this->allIndicies.insert(familyIndex);

        if (presentSupport)
            this->presentFamilies.insert(familyIndex);

        if (queueSupport & vk::QueueFlagBits::eGraphics)
            this->graphicsFamilies.insert(familyIndex);

        if (queueSupport & vk::QueueFlagBits::eTransfer)
            this->transferFamilies.insert(familyIndex);

        if (queueSupport & vk::QueueFlagBits::eCompute)
            this->computeFamilies.insert(familyIndex);
    }

    // check if queue families are all seperate -- this means more parallel work
    bool isOptimalSupport() const
    {
        if (!this->isFullySupported())
        {
            return false;
        }

        // select presentation
        std::vector<uint32_t> base = std::vector<uint32_t>();

        // select queue with present + graphics
        {
            std::vector<uint32_t> found;
            std::set_intersection(this->graphicsFamilies.begin(), this->graphicsFamilies.end(),
                                  this->presentFamilies.begin(), this->presentFamilies.end(),
                                  std::back_inserter(found));
            if (found.size() > 0)
                base.push_back(found[0]);
            else
                return false;
        }

        // select compute
        {
            std::vector<uint32_t> found;
            std::set_difference(this->computeFamilies.begin(), this->computeFamilies.end(), base.begin(), base.end(),
                                std::back_inserter(found));
            if (found.size() > 0)
                base.push_back(found[0]);
            else
                return false;
        }

        // select transfer
        {
            std::vector<uint32_t> found;
            std::set_difference(this->transferFamilies.begin(), this->transferFamilies.end(), base.begin(), base.end(),
                                std::back_inserter(found));
            if (found.size() > 0)
                base.push_back(found[0]);
            else
                return false;
        }

        return true;
    }

    bool isFullySupported() const
    {
        if (this->graphicsFamilies.size() > 0 && this->presentFamilies.size() > 0 &&
            this->transferFamilies.size() > 0 && this->computeFamilies.size() > 0)
        {
            return true;
        }
        return false;
    }

    bool isSuitable() const
    {
        return this->graphicsFamilies.size() > 0 && this->presentFamilies.size() > 0;
    }

    uint32_t getNumQueuesForIndex(const uint32_t &index)
    {
        return this->familyIndexQueueCount[index];
    }

    vk::QueueFlags getSupportForIndex(const uint32_t &index)
    {
        return this->familyIndexQueueSupport[index];
    }

    std::set<uint32_t> getUniques() const
    {
        return this->allIndicies;
    }

    bool getSupportsPresentForIndex(const uint32_t &index)
    {
        return this->presentFamilies.find(index) != this->presentFamilies.end();
    }

  private:
    std::set<uint32_t> allIndicies = std::set<uint32_t>();
    std::unordered_map<uint32_t, vk::QueueFlags> familyIndexQueueSupport =
        std::unordered_map<uint32_t, vk::QueueFlags>();
    std::unordered_map<uint32_t, uint32_t> familyIndexQueueCount = std::unordered_map<uint32_t, uint32_t>();
    std::set<uint32_t> graphicsFamilies = std::set<uint32_t>();
    std::set<uint32_t> presentFamilies = std::set<uint32_t>();
    std::set<uint32_t> transferFamilies = std::set<uint32_t>();
    std::set<uint32_t> computeFamilies = std::set<uint32_t>();
};

class StarDevice
{
  public:
    class QueueOwnershipTracker
    {
      public:
        QueueOwnershipTracker(std::vector<StarQueueFamily> regQueueFamilies)
            : regQueueFamilies(regQueueFamilies),
              isQueueAvailable(std::vector<std::vector<bool>>(regQueueFamilies.size()))
        {
            for (int i = 0; i < this->regQueueFamilies.size(); i++)
            {
                this->isQueueAvailable.at(i) = std::vector<bool>(this->regQueueFamilies.at(i).getQueueCount(), true);
            }
        }

        std::optional<StarQueue> giveMeQueueWithProperties(const vk::QueueFlags &capabilities,
                                                           const bool &presentationSupport = false)
        {
            for (int i = 0; i < this->regQueueFamilies.size(); i++)
            {
                if (this->regQueueFamilies.at(i).doesSupport(capabilities, presentationSupport))
                {
                    for (int j = 0; j < this->isQueueAvailable.at(i).size(); j++)
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
            for (int i = 0; i < this->regQueueFamilies.size(); i++)
            {
                if (this->regQueueFamilies.at(i).getQueueFamilyIndex() == familyIndex &&
                    this->regQueueFamilies.at(i).doesSupport(capabilities, presentationSupport))
                {
                    for (int j = 0; j < this->isQueueAvailable.at(i).size(); j++)
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

            for (int i = 0; i < this->regQueueFamilies.size(); i++)
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
            for (int i = 0; i < this->regQueueFamilies.size(); i++)
            {
                indices.emplace_back(this->regQueueFamilies.at(i).getQueueFamilyIndex());
            }

            return indices;
        }

      private:
        std::vector<StarQueueFamily> regQueueFamilies = std::vector<StarQueueFamily>();
        std::vector<std::vector<bool>> isQueueAvailable = std::vector<std::vector<bool>>();
    };

    static std::unique_ptr<StarDevice> New(StarWindow &window, std::set<star::Rendering_Features> requiredFeatures);

    virtual ~StarDevice();

    // Not copyable or movable
    StarDevice(const StarDevice &) = delete;
    StarDevice &operator=(const StarDevice &) = delete;
    StarDevice(StarDevice &&) = delete;
    StarDevice &operator=(StarDevice &&) = delete;

    QueueFamilyIndicies findPhysicalQueueFamilies()
    {
        assert(this->physicalDevice && this->surface);

        return FindQueueFamilies(this->physicalDevice, *this->surface);
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
    SwapChainSupportDetails getSwapChainSupportDetails()
    {
        assert(this->physicalDevice && this->surface);

        return QuerySwapchainSupport(this->physicalDevice, *this->surface);
    }
    vk::PhysicalDevice getPhysicalDevice()
    {
        return this->physicalDevice;
    }
    inline vk::Device &getDevice()
    {
        return this->vulkanDevice;
    }
    vk::SurfaceKHR getSurface()
    {
        return this->surface.get();
    }
    vk::Instance &getInstance()
    {
        return this->instance;
    }
    Allocator &getAllocator()
    {
        assert(this->allocator &&
               "Default allocator should have been created during startup. Something has gone wrong.");
        return *this->allocator;
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
    StarDevice(StarWindow &window, std::set<star::Rendering_Features> requiredFeatures);

#ifdef NDEBUG
    const bool enableValidationLayers = false;
    const std::vector<const char *> validationLayers = {};
#else
    const bool enableValidationLayers = true;
    const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
#endif
    vk::Instance instance;
    vk::Device vulkanDevice;
    std::unique_ptr<star::Allocator> allocator;
    vk::PhysicalDevice physicalDevice = VK_NULL_HANDLE;
    vk::UniqueSurfaceKHR surface;
    StarWindow &starWindow;

    std::vector<std::unique_ptr<StarQueueFamily>> extraFamilies = std::vector<std::unique_ptr<StarQueueFamily>>();
    std::unique_ptr<QueueOwnershipTracker> currentDeviceQueues = std::unique_ptr<QueueOwnershipTracker>();

    std::unique_ptr<StarQueue> defaultQueue = nullptr;
    std::unique_ptr<StarQueue> dedicatedComputeQueue = nullptr;
    std::unique_ptr<StarQueue> dedicatedTransferQueue = nullptr;

    std::shared_ptr<StarCommandPool> defaultCommandPool = nullptr;
    std::shared_ptr<StarCommandPool> transferCommandPool = nullptr;
    std::shared_ptr<StarCommandPool> computeCommandPool = nullptr;

    const std::vector<const char *> requiredDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME, // image presentation is not built into the vulkan core...need to enable it
                                         // through an extension
        VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME, VK_KHR_BIND_MEMORY_2_EXTENSION_NAME,
        VK_EXT_MEMORY_BUDGET_EXTENSION_NAME, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_EXT_DYNAMIC_RENDERING_UNUSED_ATTACHMENTS_EXTENSION_NAME};

    vk::PhysicalDeviceFeatures requiredDeviceFeatures{};

#if __APPLE__
    bool isMac = true;
    std::vector<const char *> platformInstanceRequiredExtensions = {
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, "VK_KHR_portability_enumeration"};
#else
    bool isMac = false;
    std::vector<const char *> platformInstanceRequiredExtensions = {};
#endif

    // Create the vulkan instance machine
    void createInstance();

    // Pick a proper physical GPU that matches the required extensions
    void pickPhysicalDevice();
    // Create a logical device to communicate with the physical device
    void createLogicalDevice();

    void createAllocator();

    /* Helper Functions */

    // Helper function to test each potential GPU device
    static bool IsDeviceSuitable(const std::vector<const char *> &requiredDeviceExtensions,
                                 const vk::PhysicalDeviceFeatures &requiredDeviceFeatures,
                                 const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface);

    // Get the extensions required by the system
    std::vector<const char *> getRequiredExtensions();

    /// <summary>
    /// Check if validation layers are supported and create the layers if needed. Will create layers for debugging
    /// builds only.
    /// </summary>
    /// <returns></returns>
    bool CheckValidationLayerSupport(const std::vector<const char *> &validationLayers);

    /// <summary>
    /// Find what queues are available for the device
    /// Queues support different types of commands such as : processing compute commands or memory transfer commands
    /// </summary>
    static QueueFamilyIndicies FindQueueFamilies(const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface);

    // void populateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo);

    void hasGlfwRequiredInstanceExtensions();

    /// <summary>
    /// Check if the given device supports required extensions.
    /// </summary>
    static bool CheckDeviceExtensionSupport(const vk::PhysicalDevice &device,
                                            const std::vector<const char *> &requiredDeviceExtensions);

    /// <summary>
    /// Request specific details about swap chain support for a given device
    /// </summary>
    static SwapChainSupportDetails QuerySwapchainSupport(const vk::PhysicalDevice &device,
                                                         const vk::SurfaceKHR &surface);

  private:
    static bool DoesDeviceSupportPresentation(vk::PhysicalDevice device, const vk::SurfaceKHR &surface);
};
} // namespace star