//wrapper class for textures 
#pragma once

#include "StarDevice.hpp"
#include "StarBuffer.hpp"
#include "StarDescriptorBuilders.hpp"
#include "ConfigFile.hpp"

#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>
#include <sstream>

#include <memory>
#include <string>
#include <optional>

namespace star {
class StarTexture {
public:
	/// <summary>
	/// Options to be used when creating a new texture
	/// </summary>
	/// <returns></returns>
	struct TextureCreateSettings {
		TextureCreateSettings(const int& width, 
			const int& height, const int& channels, 
			const int& depth, const int& byteDepth,
			const vk::ImageUsageFlags& imageUsage, const vk::Format& imageFormat,
			const vk::ImageAspectFlags& imageAspectFlags, const VmaMemoryUsage& memoryUsage,
			const VmaAllocationCreateFlags& allocationCreateFlags, const vk::ImageLayout& initialLayout, 
			const bool& isMutable, const bool& createSampler) 
			: width(width), height(height), channels(channels), depth(depth), byteDepth(byteDepth),
			imageUsage(imageUsage), imageFormat(imageFormat), 
			allocationCreateFlags(allocationCreateFlags), memoryUsage(memoryUsage), 
			isMutable(isMutable), createSampler(createSampler), initialLayout(initialLayout), aspectFlags(imageAspectFlags){ }

		~TextureCreateSettings() = default; 

		bool createSampler = true; 
		bool isMutable = false;
		int height, width, channels, depth, byteDepth = 0; 
		vk::ImageUsageFlags imageUsage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
		vk::Format imageFormat = vk::Format::eR8G8B8A8Srgb;
		VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
		VmaAllocationCreateFlags allocationCreateFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT & VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;
		vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor;
		vk::ImageLayout initialLayout = vk::ImageLayout::eShaderReadOnlyOptimal; 

	private:
		TextureCreateSettings() = default;
	}; 

	StarTexture(TextureCreateSettings settings,
		const vk::Image& image) 
		: creationSettings(settings), textureImage(image), layout(settings.initialLayout)
	{
	}; 

	StarTexture(TextureCreateSettings settings)
		: creationSettings(settings)
	{
	};

	virtual void prepRender(StarDevice& device);

	virtual ~StarTexture() = default;

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

	int getWidth() const { return this->creationSettings.width; };
	int getHeight() const { return this->creationSettings.height; };
	int getChannels() const { return this->creationSettings.channels; }
	int getDepth() const { return this->creationSettings.depth; }
	TextureCreateSettings getCreationSettings() const { return this->creationSettings; }
	//std::vector<vk::Format> getFormats() const {
	//	std::vector<vk::Format> formats; 
	//	for (auto& info : this->imageViews) {
	//		formats.push_back(info.first); 
	//	}
	//}
protected:
	TextureCreateSettings creationSettings;
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