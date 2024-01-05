#pragma once

#include "Enums.hpp"
#include "StarWindow.hpp"

#include <optional>
#include <unordered_set>
#include <set>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <iostream>

namespace star {
struct SwapChainSupportDetails {
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> presentModes;
};

struct QueueFamilyIndicies {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;
	std::optional<uint32_t> transferFamily;
	std::optional<uint32_t> computeFamily; 

	//check if queue families are all seperate -- this means more parallel work
	bool isOptimalSupport(){
		if ((graphicsFamily.has_value() && presentFamily.has_value() && transferFamily.has_value() && computeFamily.has_value())
			&& (graphicsFamily.value() != transferFamily.value() && graphicsFamily.value() != computeFamily.value())) {
				return true;
		}
		return false; 
	}
	bool isFullySupported() {
		if (graphicsFamily.has_value() && presentFamily.has_value() && transferFamily.has_value() && computeFamily.has_value()) {
			return true; 
		}
		return false; 
	}
	bool isSuitable() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

class StarDevice {
public:
	static std::unique_ptr<StarDevice> New(StarWindow& window, std::vector<star::Rendering_Features> requiredFeatures); 

	virtual ~StarDevice();

	// Not copyable or movable
	StarDevice(const StarDevice&) = delete;
	StarDevice& operator=(const StarDevice&) = delete;
	StarDevice(StarDevice&&) = delete;
	StarDevice& operator=(StarDevice&&) = delete;

	QueueFamilyIndicies findPhysicalQueueFamilies() { return findQueueFamilies(this->physicalDevice); }

	/// <summary>
	/// Check the hardware to make sure that the supplied formats are compatible with the current system. 
	/// </summary>
	/// <param name="candidates">List of possible formats to check</param>
	/// <param name="tiling"></param>
	/// <param name="features"></param>
	/// <returns></returns>
	vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);

	vk::CommandPool& getCommandPool(star::Command_Buffer_Type type);

	vk::Queue& getQueue(star::Command_Buffer_Type type);

#pragma region getters
	SwapChainSupportDetails getSwapChainSupportDetails() { return querySwapChainSupport(this->physicalDevice); }
	vk::CommandPool getGraphicsCommandPool() { return this->graphicsCommandPool; }
	vk::PhysicalDevice getPhysicalDevice() { return this->physicalDevice; }
	vk::Device getDevice() { return this->vulkanDevice; }
	vk::SurfaceKHR getSurface() { return this->surface.get(); }
	vk::Queue getGraphicsQueue() { return this->graphicsQueue; }
	vk::Queue getPresentQueue() { return this->presentQueue; }
	vk::Queue getTransferQueue() { 
		if (this->transferQueue.has_value())
			return this->transferQueue.value();
		else
			return this->graphicsQueue; 
		};
	vk::Queue getComputeQueue() {
		if (this->computeQueue.has_value())
			return this->computeQueue.value();
		else
			return this->graphicsQueue;
	}
#pragma endregion

#pragma region helperFunctions
	void createPool(uint32_t queueFamilyIndex, vk::CommandPoolCreateFlagBits flags, vk::CommandPool& pool);
	/// <summary>
	/// Create a buffer with the given arguments
	/// </summary>
	void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usageFlags, vk::MemoryPropertyFlags properties,
		vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);
	/// <summary>
	/// Helper function to execute single time use command buffers
	/// </summary>
	/// <param name="useTransferPool">Should command be submitted to the transfer command pool. Will be submitted to the graphics pool otherwise.</param>
	/// <returns></returns>
	vk::CommandBuffer beginSingleTimeCommands(bool useTransferPool = false);
	/// <summary>
	/// Helper function to end execution of single time use command buffer
	/// </summary>
	/// <param name="commandBuffer"></param>
	/// <param name="useTransferPool">Was command buffer submitted to the transfer pool. Assumed graphics pool otherwise.</param>
	void endSingleTimeCommands(vk::CommandBuffer commandBuffer, bool useTransferPool = false, vk::Semaphore* signalFinishSemaphore = nullptr);

	void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size, const vk::DeviceSize dstOffset=0);

	/// <summary>
	/// Copy a buffer to an image.
	/// </summary>
	/// <param name="buffer"></param>
	/// <param name="image"></param>
	/// <param name="width"></param>
	/// <param name="height"></param>
	void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);

	void createImageWithInfo(const vk::ImageCreateInfo& imageInfo, vk::MemoryPropertyFlags properties, vk::Image& image,
		vk::DeviceMemory& imageMemory);

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
	StarDevice(StarWindow& window, std::vector<star::Rendering_Features> requiredFeatures);

#ifdef NDEBUG 
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif    

	bool hasDedicatedTransferQueue = false;

	vk::Instance instance;
	vk::Device vulkanDevice;
	vk::PhysicalDevice physicalDevice = VK_NULL_HANDLE;
	vk::UniqueSurfaceKHR surface;
	StarWindow& starWindow;

	//vulkan command storage
	vk::CommandPool graphicsCommandPool;
	vk::CommandPool transferCommandPool;
	std::vector<vk::CommandBuffer> transferCommandBuffers;
	vk::CommandPool computeCommandPool; 
	vk::CommandPool tempCommandPool; //command pool for temporary use in small operations

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME//image presentation is not built into the vulkan core...need to enable it through an extension
	};

	vk::PhysicalDeviceFeatures requiredDeviceFeatures{};

	//queue family information
	vk::Queue graphicsQueue, presentQueue; 
	std::optional<vk::Queue> transferQueue, computeQueue;

#if __APPLE__
	bool isMac = true;
	std::vector<const char*> platformInstanceRequiredExtensions = {
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
		// VK_KHR_SURFACE_EXTENSION_NAME,
		"VK_KHR_portability_enumeration"
	};
#else
	bool isMac = false;
	std::vector<const char*> platformInstanceRequiredExtensions = { };
#endif

	void prepRequiredFeatures(const std::vector<star::Rendering_Features>& features); 

	//Create the vulkan instance machine 
	void createInstance();

	//Pick a proper physical GPU that matches the required extensions
	void pickPhysicalDevice();
	//Create a logical device to communicate with the physical device 
	void createLogicalDevice();
	/// <summary>
	/// Create command pools which will contain all predefined draw commands for later use in vulkan main loop
	/// </summary>
	void createCommandPool();

	/* Helper Functions */

	//Helper function to test each potential GPU device 
	bool isDeviceSuitable(vk::PhysicalDevice physicalDevice);

	//Get the extensions required by the system 
	std::vector<const char*> getRequiredExtensions();

	/// <summary>
	/// Check if validation layers are supported and create the layers if needed. Will create layers for debugging builds only.
	/// </summary>
	/// <returns></returns>
	bool checkValidationLayerSupport();

	/// <summary>
	/// Find what queues are available for the device
	/// Queues support different types of commands such as : processing compute commands or memory transfer commands
	/// </summary>  
	QueueFamilyIndicies findQueueFamilies(vk::PhysicalDevice device);

	//void populateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo);

	void hasGlfwRequiredInstanceExtensions();

	/// <summary>
	/// Check if the given device supports required extensions.
	/// </summary>
	bool checkDeviceExtensionSupport(vk::PhysicalDevice device);

	/// <summary>
	/// Request specific details about swap chain support for a given device
	/// </summary>
	SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device);

private: 

};
}