#pragma once

#include "StarCamera.hpp"
#include "StarRenderer.hpp"
#include "StarScene.hpp"
#include "StarWindow.hpp"
#include "StarDescriptorBuilders.hpp"
#include "LightBufferObject.hpp"
#include "InteractionSystem.hpp"
#include "StarObject.hpp"
#include "StarCommandBuffer.hpp"
#include "GlobalInfo.hpp"
#include "StarShaderInfo.hpp"

#include "MapManager.hpp"
#include "StarRenderGroup.hpp"
#include "ShaderManager.hpp"
#include "TextureManager.hpp"
#include "LightManager.hpp"
#include "ManagerDescriptorPool.hpp"
#include "ManagerCommandBuffer.hpp"
#include "RenderResourceModifier.hpp"
#include "CommandBufferModifier.hpp"
#include "DescriptorModifier.hpp"
#include "ManagerBuffer.hpp"
#include "StarImage.hpp"

#include "Light.hpp"

#include <chrono>
#include <memory>
#include <optional>
#include <vulkan/vulkan.hpp>

namespace star {
class SceneRenderer : public StarRenderer, public CommandBufferModifier, private RenderResourceModifier, private DescriptorModifier {
public:
	SceneRenderer(StarScene& scene);

	virtual ~SceneRenderer() = default;

	virtual void prepare(StarDevice& device, const vk::Extent2D& swapChainExtent, 
		const int& numFramesInFlight);

	StarDescriptorSetLayout& getGlobalShaderInfo() { return *this->globalSetLayout; }

	virtual RenderingTargetInfo getRenderingInfo() { return *this->renderToTargetInfo; }
	
	std::vector<std::unique_ptr<star::StarImage>>* getRenderToColorImages() { return &this->renderToImages; }
	std::vector<std::unique_ptr<star::StarImage>>* getRenderToDepthImages() { return &this->renderToDepthImages; }
protected:
	StarScene& scene; 

	std::unique_ptr<vk::Extent2D> swapChainExtent = std::unique_ptr<vk::Extent2D>();
	
	std::unique_ptr<RenderingTargetInfo> renderToTargetInfo = std::unique_ptr<RenderingTargetInfo>(); 

	std::vector<std::unique_ptr<star::StarImage>> renderToImages = std::vector<std::unique_ptr<star::StarImage>>();
	std::vector<vk::Framebuffer> renderToFramebuffers = std::vector<vk::Framebuffer>();

	//depth testing storage 
	std::vector<std::unique_ptr<star::StarImage>> renderToDepthImages = std::vector<std::unique_ptr<star::StarImage>>();

	//Sync obj storage 
	std::vector<vk::Semaphore> imageAvailableSemaphores;

	//storage for multiple buffers for each swap chain image  
	//std::vector<vk::DescriptorSet> globalDescriptorSets;
	std::unique_ptr<StarShaderInfo> globalShaderInfo; 
	std::vector<vk::DescriptorSet> lightDescriptorSets;

	std::shared_ptr<StarDescriptorSetLayout> globalSetLayout{};
	std::vector<std::unique_ptr<StarRenderGroup>> renderGroups; 

	virtual vk::Format getCurrentRenderToImageFormat() = 0; 

	virtual std::vector<std::unique_ptr<StarImage>> createRenderToImages(StarDevice& device, const int& numFramesInFlight);

	virtual std::vector<std::unique_ptr<StarImage>> createRenderToDepthImages(StarDevice& device, const int& numFramesInFlight);

	/// <summary>
	/// Create vertex buffer + index buffers + any rendering groups for operations
	/// </summary>
	virtual void createRenderingGroups(StarDevice& device, const vk::Extent2D& swapChainExtent, const int& numFramesInFlight, StarShaderInfo::Builder builder);

	vk::ImageView createImageView(StarDevice& device, vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags);
	
	virtual StarShaderInfo::Builder manualCreateDescriptors(StarDevice& device, const int& numFramesInFlight);

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
	void createDescriptors(star::StarDevice& device, const int& numFramesInFlight) override;
};
}