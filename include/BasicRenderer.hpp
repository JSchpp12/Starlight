#pragma once

#include "Camera.hpp"
#include "StarRenderer.hpp"
#include "StarWindow.hpp"
#include "StarDescriptors.hpp"
#include "LightBufferObject.hpp"
#include "ObjectManager.hpp"
#include "InteractionSystem.hpp"
#include "StarObject.hpp"

#include "MapManager.hpp"
#include "StarSystemRenderPointLight.hpp"
#include "StarSystemRenderObj.hpp"
#include "ShaderManager.hpp"
#include "TextureManager.hpp"
#include "LightManager.hpp"
#include "RenderOptions.hpp"

#include "Light.hpp"

#include <chrono>
#include <memory>
#include <vulkan/vulkan.hpp>

namespace star {
class BasicRenderer : public StarRenderer {
public:
	class Builder {
	public:
		Builder(StarWindow& window, 
			MapManager& mapManager, ShaderManager& shaderManager, 
			Camera& camera, RenderOptions& renderOptions, StarDevice& device)
			: window(window), mapManager(mapManager), shaderManager(shaderManager), 
			camera(camera), renderOptions(renderOptions), device(device) {};

		Builder& addLight(Light& light) {
			this->lightList.push_back(light);
			return *this;
		}

		Builder& addObject(StarObject& gameObject) {
			objectList.emplace_back(gameObject); 
			return *this; 
		}

		std::unique_ptr<BasicRenderer> build() {
			auto newRenderer = std::unique_ptr<BasicRenderer>(new BasicRenderer(window, 
				mapManager, shaderManager, lightList, objectList, camera, renderOptions, device));
			newRenderer->prepare();
			return newRenderer;
		}

	private:
		StarWindow& window;
		MapManager& mapManager;
		ShaderManager& shaderManager; 
		Camera& camera;
		StarDevice& device; 
		RenderOptions& renderOptions; 
		std::vector<std::reference_wrapper<Light>> lightList;
		std::vector<std::reference_wrapper<StarObject>> objectList; 
	}; 

	virtual ~BasicRenderer();

	virtual void draw(); 

protected:
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
	MapManager& mapManager;
	ShaderManager& shaderManager; 
	RenderOptions& renderOptions; 

	std::vector<std::reference_wrapper<Light>> lightList;
	std::vector<std::reference_wrapper<StarObject>> objectList; 
	std::vector<std::unique_ptr<StarSystemRenderObject>> RenderSysObjs;
	//std::unique_ptr<StarSystemRenderPointLight> lightRenderSys;

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

	//depth testing storage 
	vk::Image depthImage;
	vk::DeviceMemory depthImageMemory;
	vk::ImageView depthImageView;

	//how many frames will be sent through the pipeline
	const int MAX_FRAMES_IN_FLIGHT = 2;
	//tracker for which frame is being processed of the available permitted frames
	size_t currentFrame = 0;

	bool frameBufferResized = false; //explicit declaration of resize, used if driver does not trigger VK_ERROR_OUT_OF_DATE


	BasicRenderer(StarWindow& window, 
		MapManager& mapManager, ShaderManager& shaderManager, std::vector<std::reference_wrapper<Light>> inLightList, 
		std::vector<std::reference_wrapper<StarObject>> objectList, Camera& camera, RenderOptions& renderOptions, StarDevice& device);

	virtual void prepare();

	void updateUniformBuffer(uint32_t currentImage);

	virtual void cleanup();

	virtual void cleanupSwapChain();

	/// <summary>
	/// If the swapchain is no longer compatible, it must be recreated.
	/// </summary>
	void recreateSwapChain();

	/// <summary>
	/// Create a swap chain that will be used in rendering images
	/// </summary>
	void createSwapChain();

	/// <summary>
	/// Create an image view object for use in the rendering pipeline
	/// 'Image View': describes how to access an image and what part of an image to access
	/// </summary>
	void createImageViews();

	vk::ImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlagBits aspectFlags);

	/// <summary>
	/// Create a rendering pass object which will tell vulkan information about framebuffer attachments:
	/// number of color and depth buffers, how many samples to use for each, how to handle contents
	/// </summary>
	void createRenderPass();

	/// <summary>
	/// Create the depth images that will be used by vulkan to run depth tests on fragments. 
	/// </summary>
	void createDepthResources();

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
	void createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlagBits properties, vk::Image& image, vk::DeviceMemory& imageMemory);

	/// <summary>
	/// Create framebuffers that will hold representations of the images in the swapchain
	/// </summary>
	void createFramebuffers();

	/// <summary>
	/// Create a buffer to hold the UBO data for each shader. Create a buffer for each swap chain image
	/// </summary>
	void createRenderingBuffers();

	/// <summary>
	/// Allocate and record the commands for each swapchain image
	/// </summary>
	void createCommandBuffers();

	/// <summary>
	/// Create semaphores that are going to be used to sync rendering and presentation queues
	/// </summary>
	void createSemaphores();

	/// <summary>
	/// Fences are needed for CPU-GPU sync. Creates these required objects
	/// </summary>
	void createFences();

	/// <summary>
	/// Create tracking information in order to link fences with the swap chain images using 
	/// </summary>
	void createFenceImageTracking();
private:

};
}