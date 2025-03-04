//wrapper class for textures 
#pragma once

#include "StarDevice.hpp"
#include "StarBuffer.hpp"
#include "StarDescriptorBuilders.hpp"
#include "ConfigFile.hpp"
#include "StarTexture.hpp"

#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>
#include <sstream>

#include <memory>
#include <string>
#include <optional>

namespace star {
class StarImage : public StarTexture{
public:
	StarImage(TextureCreateSettings settings,
		const vk::Image& image) 
		: StarTexture(settings), textureImage(image), layout(settings.initialLayout)
	{
	}; 

	StarImage(TextureCreateSettings settings)
		: StarTexture(settings)
	{
	};

	virtual void prepRender(StarDevice& device);

	virtual ~StarImage() = default;

	void cleanupRender(StarDevice& device); 

	void createTextureImageView(StarDevice& device, const vk::Format& viewFormat, const vk::ImageAspectFlags& aspectFlags);

	vk::ImageView getImageView(vk::Format* requestedFormat = nullptr) const;
	vk::Sampler getSampler() const { return this->textureSampler ? *this->textureSampler : VK_NULL_HANDLE; }
	vk::Image getImage() const { return this->textureImage; }
	vk::ImageLayout getCurrentLayout() const { return this->layout; }
	void overrideImageLayout(vk::ImageLayout newLayout) { this->layout = newLayout; }

	static void createImage(StarDevice& device, int width, int height, int depth, vk::Format format, vk::ImageTiling tiling,
		vk::ImageUsageFlags usage, const VmaMemoryUsage& memoryUsage, const VmaAllocationCreateFlags& allocationCreateFlags, 
		vk::Image& image, VmaAllocation& imageMemory, bool isMutable, vk::MemoryPropertyFlags* optional_setRequiredMemoryPropertyFlags = nullptr);

	void transitionLayout(vk::CommandBuffer& commandBuffer, vk::ImageLayout newLayout, vk::AccessFlags srcFlags, 
		vk::AccessFlags dstFlags, vk::PipelineStageFlags sourceStage,
		vk::PipelineStageFlags dstStage);


	//std::vector<vk::Format> getFormats() const {
	//	std::vector<vk::Format> formats; 
	//	for (auto& info : this->imageViews) {
	//		formats.push_back(info.first); 
	//	}
	//}
protected:
	vk::ImageLayout layout = vk::ImageLayout::eUndefined;
	vk::Image textureImage = vk::Image();

	std::unique_ptr<VmaAllocation> imageAllocation = std::unique_ptr<VmaAllocation>(); 
	std::unordered_map<vk::Format, vk::ImageView> imageViews	= std::unordered_map<vk::Format, vk::ImageView>();				//image view: describe to vulkan how to access an image
	std::unique_ptr<vk::Sampler> textureSampler									= std::unique_ptr<vk::Sampler>();					//using sampler to apply filtering or other improvements over raw texel access

	virtual std::unique_ptr<StarBuffer> loadImageData(StarDevice& device) {
		return nullptr; 
	};

	virtual void createImage(StarDevice& device);

	void transitionImageLayout(StarDevice& device, vk::Image image, vk::Format format, vk::ImageLayout oldLayout,
		vk::ImageLayout newLayout);

	void createImageSampler(StarDevice& device);

	vk::ImageView createImageView(StarDevice& device, vk::Image image, vk::Format format, const vk::ImageAspectFlags& aspectFlags);
};
}