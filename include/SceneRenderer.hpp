#pragma once

#include "StarCamera.hpp"
#include "StarRenderer.hpp"
#include "StarWindow.hpp"
#include "StarDescriptors.hpp"

#include "LightBufferObject.hpp"
#include "InteractionSystem.hpp"
#include "StarObject.hpp"
#include "StarCommandBuffer.hpp"

#include "MapManager.hpp"
#include "StarSystemRenderPointLight.hpp"
#include "StarRenderGroup.hpp"
#include "ShaderManager.hpp"
#include "TextureManager.hpp"
#include "LightManager.hpp"
#include "ManagerDescriptorPool.hpp"
#include "ManagerCommandBuffer.hpp"
#include "RenderResourceModifier.hpp"
#include "CommandBufferModifier.hpp"
#include "DescriptorModifier.hpp"
#include "Texture.hpp"

#include "Light.hpp"

#include <chrono>
#include <memory>
#include <optional>
#include <vulkan/vulkan.hpp>

namespace star {
class SceneRenderer : public StarRenderer, public CommandBufferModifier, private RenderResourceModifier, private DescriptorModifier {
public:
	SceneRenderer(std::vector<std::unique_ptr<Light>>& lightList,
		std::vector<std::reference_wrapper<StarObject>> objectList, StarCamera& camera);

	virtual ~SceneRenderer() = default;

	virtual void prepare(StarDevice& device, const vk::Extent2D& swapChainExtent, 
		const int& numFramesInFlight);

	StarDescriptorSetLayout& getGlobalDescriptorLayout() { return *this->globalSetLayout; }

	virtual RenderingTargetInfo getRenderingInfo() { return *this->renderToTargetInfo; }

	star::Texture& getRenderToColorImage(const int& imageIndex) { return *this->renderToImages[imageIndex]; }
	std::vector<vk::Image>* getRenderToDepthImages() { return &this->renderToDepthImages; }	
protected:
	struct GlobalUniformBufferObject {
		alignas(16) glm::mat4 proj;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 inverseView;              //used to extrapolate camera position, can be used to convert from camera to world space
		uint32_t numLights;                             //number of lights in render
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
	std::unique_ptr<vk::Extent2D> swapChainExtent = std::unique_ptr<vk::Extent2D>();
	
	std::unique_ptr<RenderingTargetInfo> renderToTargetInfo = std::unique_ptr<RenderingTargetInfo>(); 

	std::vector<std::unique_ptr<star::Texture>> renderToImages = std::vector<std::unique_ptr<star::Texture>>(); 
	std::vector<vk::Framebuffer> renderToFramebuffers = std::vector<vk::Framebuffer>();

	//depth testing storage 
	std::vector<vk::Image> renderToDepthImages = std::vector<vk::Image>(); 
	std::vector<VmaAllocation> renderToDepthImageMemory = std::vector<VmaAllocation>();
	std::vector<vk::ImageView> renderToDepthImageViews = std::vector<vk::ImageView>(); 

	std::vector<std::unique_ptr<Light>>& lightList;
	std::vector<std::reference_wrapper<StarObject>> objectList; 

	//Sync obj storage 
	std::vector<vk::Semaphore> imageAvailableSemaphores;

	//storage for multiple buffers for each swap chain image  
	std::vector<std::unique_ptr<StarBuffer>> globalUniformBuffers;
	std::vector<vk::DescriptorSet> globalDescriptorSets;
	std::vector<std::unique_ptr<StarBuffer>> lightBuffers;
	std::vector<vk::DescriptorSet> lightDescriptorSets;

	std::unique_ptr<StarDescriptorSetLayout> globalSetLayout{};
	std::vector<std::unique_ptr<StarRenderGroup>> renderGroups; 

	virtual vk::Format getCurrentRenderToImageFormat() = 0; 

	virtual std::vector<std::unique_ptr<Texture>> createRenderToImages(StarDevice& device, const int& numFramesInFlight);

	/// <summary>
	/// Create an image view object for use in the rendering pipeline
	/// 'Image View': describes how to access an image and what part of an image to access
	/// </summary>
	virtual void createImageViews(StarDevice& device, const int& numImages, const vk::Format& imageFormat);

	/// <summary>
	/// Create vertex buffer + index buffers + any rendering groups for operations
	/// </summary>
	virtual void createRenderingGroups(StarDevice& device, const vk::Extent2D& swapChainExtent, const int& numFramesInFlight);

	virtual void updateUniformBuffer(uint32_t currentImage);

	vk::ImageView createImageView(StarDevice& device, vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags);
	
	/// <summary>
	/// Create descriptor pools for the descriptors used by the main rendering engine.
	/// </summary>
	virtual void createDescriptors(StarDevice& device, const int& numFramesInFlight);

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
	virtual void createImage(StarDevice& device, uint32_t width, uint32_t height, 
		vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, 
		vk::MemoryPropertyFlags properties, vk::Image& image, 
		VmaAllocation& imageMemory);

	/// <summary>
	/// Create a buffer to hold the UBO data for each shader. Create a buffer for each swap chain image
	/// </summary>
	virtual void createRenderingBuffers(StarDevice& device, const int& numFramesInFlight);

	/// <summary>
	/// Create the depth images that will be used by vulkan to run depth tests on fragments. 
	/// </summary>
	vk::Format createDepthResources(StarDevice& device, const vk::Extent2D& swapChainExtent, const int& numFramesInFlight);

	// Inherited via CommandBufferModifier
	Command_Buffer_Type getCommandBufferType() override;
	virtual Command_Buffer_Order getCommandBufferOrder() = 0;
	virtual vk::PipelineStageFlags getWaitStages() = 0;
	virtual bool getWillBeSubmittedEachFrame() = 0;
	virtual bool getWillBeRecordedOnce() = 0;
	virtual void recordCommandBuffer(vk::CommandBuffer& commandBuffer, const int& frameInFlightIndex) override;

	virtual void prepareForSubmission(const int& frameIndexToBeDrawn);

	virtual std::optional<std::function<void(const int&)>> getBeforeBufferSubmissionCallback() override;

	// Inherited via RenderResourceModifier
	virtual void initResources(StarDevice& device, const int& numFramesInFlight, const vk::Extent2D& screensize) override;

	virtual void destroyResources(StarDevice& device) override;

#pragma region helpers
	vk::Format findDepthFormat(StarDevice& device);

	vk::Viewport prepareRenderingViewport();

	virtual vk::RenderingAttachmentInfo prepareDynamicRenderingInfoColorAttachment(const int& frameInFlightIndex);

	virtual vk::RenderingAttachmentInfo prepareDynamicRenderingInfoDepthAttachment(const int& frameInFlightIndex);

	void recordPreRenderingCalls(vk::CommandBuffer& commandBuffer, const int& frameInFlightIndex);

	void recordRenderingCalls(vk::CommandBuffer& commandBuffer, const int& frameInFlightIndex);
#pragma endregion
private:
	// Inherited via DescriptorModifier
	std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(const int& numFramesInFlight) override;
};
}