#pragma once

#include "StarCamera.hpp"
#include "StarRenderer.hpp"
#include "StarWindow.hpp"
#include "StarDescriptors.hpp"
#include "LightBufferObject.hpp"
#include "ObjectManager.hpp"
#include "InteractionSystem.hpp"
#include "StarObject.hpp"
#include "StarCommandBuffer.hpp"

#include "MapManager.hpp"
#include "StarSystemRenderPointLight.hpp"
#include "StarRenderGroup.hpp"
#include "ShaderManager.hpp"
#include "TextureManager.hpp"
#include "LightManager.hpp"
#include "RenderOptions.hpp"

#include "Light.hpp"

#include <chrono>
#include <memory>
#include <vulkan/vulkan.hpp>

namespace star {
class SwapChainRenderer : public StarRenderer {
public:
	SwapChainRenderer(StarWindow& window, std::vector<std::unique_ptr<Light>>& lightList,
		std::vector<std::reference_wrapper<StarObject>> objectList, StarCamera& camera, RenderOptions& renderOptions, StarDevice& device);

	virtual ~SwapChainRenderer();

	void pollEvents(); 

	virtual void prepare() override;

	virtual void submit() override; 

protected:
	struct RenderOptionsObject {
		bool drawMatAmbient;
	};

	struct GlobalUniformBufferObject {
		alignas(16) glm::mat4 proj;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 inverseView;              //used to extrapolate camera position, can be used to convert from camera to world space
		uint32_t numLights;                             //number of lights in render
		alignas(4) uint32_t renderOptions;
	};

	struct LightBufferObject {
		glm::vec4 position = glm::vec4(1.0f);
		glm::vec4 direction = glm::vec4(1.0f);     //direction in which the light is pointing
		glm::vec4 ambient = glm::vec4(1.0f);
		glm::vec4 diffuse = glm::vec4(1.0f);
		glm::vec4 specular = glm::vec4(1.0f);
		//controls.x = inner cutoff diameter 
		//controls.y = outer cutoff diameter
		glm::vec4 controls = glm::vec4(0.0f);       //container for single float values
		//settings.x = enabled
		//settings.y = type
		glm::uvec4 settings = glm::uvec4(0);    //container for single uint values
	};
	RenderOptions& renderOptions; 

	std::vector<std::unique_ptr<Light>>& lightList;
	std::vector<std::reference_wrapper<StarObject>> objectList; 

	std::unique_ptr<StarBuffer> vertexBuffer, indexBuffer; 

	//texture information
	vk::ImageView textureImageView;
	vk::Sampler textureSampler;
	vk::Image textureImage;
	vk::DeviceMemory textureImageMemory;

	//Sync obj storage 
	std::vector<vk::Semaphore> imageAvailableSemaphores;
	std::vector<vk::Semaphore> renderFinishedSemaphores;

	//storage for multiple buffers for each swap chain image  
	std::vector<std::unique_ptr<StarBuffer>> globalUniformBuffers;
	std::vector<vk::DescriptorSet> globalDescriptorSets;
	std::vector<std::unique_ptr<StarBuffer>> lightBuffers;
	std::vector<vk::DescriptorSet> lightDescriptorSets;

	//pipeline and dependency storage
	vk::RenderPass renderPass;

	//more swapchain info 
	vk::SwapchainKHR swapChain;
	std::vector<vk::Image> swapChainImages;
	vk::Format swapChainImageFormat;
	vk::Extent2D swapChainExtent;

	std::vector<vk::ImageView> swapChainImageViews;
	std::vector<vk::Framebuffer> swapChainFramebuffers;
	std::vector<vk::Fence> inFlightFences;
	std::vector<vk::Fence> imagesInFlight;

	std::unique_ptr<StarDescriptorPool> globalPool{};
	std::unique_ptr<StarDescriptorSetLayout> globalSetLayout{};

	std::vector<std::unique_ptr<StarRenderGroup>> renderGroups; 

	std::unique_ptr<StarCommandBuffer> graphicsCommandBuffer; 

	//depth testing storage 
	vk::Image depthImage;
	vk::DeviceMemory depthImageMemory;
	vk::ImageView depthImageView;

	//how many frames will be sent through the pipeline
	const int MAX_FRAMES_IN_FLIGHT = 2;
	//tracker for which frame is being processed of the available permitted frames
	size_t currentFrame = 0;

	bool frameBufferResized = false; //explicit declaration of resize, used if driver does not trigger VK_ERROR_OUT_OF_DATE

	/// <summary>
	/// Create vertex buffer + index buffers + any rendering groups for operations
	/// </summary>
	virtual void createRenderingGroups();

	virtual void updateUniformBuffer(uint32_t currentImage);

	virtual void cleanup();

	virtual void cleanupSwapChain();

	/// <summary>
	/// If the swapchain is no longer compatible, it must be recreated.
	/// </summary>
	virtual void recreateSwapChain();

	/// <summary>
	/// Create a swap chain that will be used in rendering images
	/// </summary>
	virtual void createSwapChain();

	/// <summary>
	/// Create an image view object for use in the rendering pipeline
	/// 'Image View': describes how to access an image and what part of an image to access
	/// </summary>
	virtual void createImageViews();

	vk::ImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlagBits aspectFlags);
	
	/// <summary>
	/// Create descriptor pools for the descriptors used by the main rendering engine.
	/// </summary>
	virtual void createDescriptors();

	/// <summary>
	/// Create a rendering pass object which will tell vulkan information about framebuffer attachments:
	/// number of color and depth buffers, how many samples to use for each, how to handle contents
	/// </summary>
	virtual void createRenderPass();

	/// <summary>
	/// Create the depth images that will be used by vulkan to run depth tests on fragments. 
	/// </summary>
	virtual void createDepthResources();

	/// <summary>
	/// Create Vulkan Image object with properties provided in function arguments. 
	/// </summary>
	/// <param name="width">Width of the image being created</param>
	/// <param name="height">Height of the image being created</param>
	/// <param name="format"></param>
	/// <param name="tiling"></param>
	/// <param name="usage"></param>
	/// <param name="properties"></param>
	/// <param name="image"></param>
	/// <param name="imageMemory"></param>
	virtual void createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlagBits properties, vk::Image& image, vk::DeviceMemory& imageMemory);

	/// <summary>
	/// Create framebuffers that will hold representations of the images in the swapchain
	/// </summary>
	virtual void createFramebuffers();

	/// <summary>
	/// Create a buffer to hold the UBO data for each shader. Create a buffer for each swap chain image
	/// </summary>
	virtual void createRenderingBuffers();

	/// <summary>
	/// Allocate and record the commands for each swapchain image
	/// </summary>
	virtual void createCommandBuffers();

	virtual void recordCommandBuffer(uint32_t bufferIndex, uint32_t imageIndex); 

	/// <summary>
	/// Create semaphores that are going to be used to sync rendering and presentation queues
	/// </summary>
	virtual void createSemaphores();

	/// <summary>
	/// Fences are needed for CPU-GPU sync. Creates these required objects
	/// </summary>
	virtual void createFences();

	/// <summary>
	/// Create tracking information in order to link fences with the swap chain images using 
	/// </summary>
	virtual void createFenceImageTracking();

#pragma region helpers
	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);

	//Look through givent present modes and pick the "best" one
	vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);

	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);

	/// <summary>
	/// Helper function -- TODO 
	/// </summary>
	/// <returns></returns>
	vk::Format findDepthFormat();
#pragma endregion
private:

};
}