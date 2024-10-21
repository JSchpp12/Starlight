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
		static TextureCreateSettings createDefault(const bool externalAllocation = false) { 
			if (externalAllocation) {
				auto settings = TextureCreateSettings(); 
				settings.memoryUsage = {}; 
				settings.allocationCreateFlags = {};
			}
			else {
				return TextureCreateSettings();
			}
		}

		TextureCreateSettings(const vk::ImageUsageFlags& imageUsage, const vk::Format& imageFormat, 
			const VmaMemoryUsage& memoryUsage, const VmaAllocationCreateFlags& allocationCreateFlags, const bool& isMutable, 
			const bool& createSampler) 
			: imageUsage(imageUsage), imageFormat(imageFormat), allocationCreateFlags(allocationCreateFlags),
			memoryUsage(memoryUsage), isMutable(isMutable), createSampler(createSampler) {}

		~TextureCreateSettings() = default; 

		bool createSampler = true; 
		bool isMutable = false;
		vk::ImageUsageFlags imageUsage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
		vk::Format imageFormat = vk::Format::eR8G8B8A8Srgb;
		VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
		VmaAllocationCreateFlags allocationCreateFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT & VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;

	private:
		TextureCreateSettings() = default; 
	}; 

	StarTexture() : createSettings(std::make_unique<TextureCreateSettings>(TextureCreateSettings::createDefault())) {}; 
	StarTexture(const vk::Image& image, const vk::ImageLayout& imageLayout, const vk::Format& format) : textureImage(image), layout(imageLayout), 
		createSettings(std::make_unique<TextureCreateSettings>(TextureCreateSettings::createDefault(true))) {
		this->createSettings->imageFormat = format; 
	}; 
	StarTexture(TextureCreateSettings& settings)
		: createSettings(std::make_unique<TextureCreateSettings>(settings)) {}; 

	virtual ~StarTexture() = default;

	virtual void prepRender(StarDevice& device);

	void cleanupRender(StarDevice& device); 

	void createTextureImageView(StarDevice& device, const vk::Format& viewFormat);

	/// <summary>
	/// Read the image from disk into memory or provide the image which is in memory
	/// </summary>
	/// <returns></returns>
	virtual std::optional<std::unique_ptr<unsigned char>> data() = 0;

	vk::ImageView getImageView(vk::Format* requestedFormat = nullptr) const;
	vk::Sampler getSampler() const { return *this->textureSampler; }
	vk::Image getImage() const { return this->textureImage; }
	vk::ImageLayout getCurrentLayout() const { return this->layout; }
	void overrideImageLayout(vk::ImageLayout newLayout) { this->layout = newLayout; }

	static void createImage(StarDevice& device, uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
		vk::ImageUsageFlags usage, const VmaMemoryUsage& memoryUsage, const VmaAllocationCreateFlags& allocationCreateFlags, 
		vk::Image& image, VmaAllocation& imageMemory, bool isMutable, vk::MemoryPropertyFlags* optional_setRequiredMemoryPropertyFlags = nullptr);

	void transitionLayout(vk::CommandBuffer& commandBuffer, vk::ImageLayout newLayout, vk::AccessFlags srcFlags, 
		vk::AccessFlags dstFlags, vk::PipelineStageFlags sourceStage,
		vk::PipelineStageFlags dstStage);

protected:
	std::unique_ptr<TextureCreateSettings> createSettings = nullptr; 

	bool isRenderReady											= false; 
	vk::ImageLayout layout = vk::ImageLayout::eUndefined;
	vk::Image textureImage = vk::Image();

	std::unique_ptr<VmaAllocation> imageAllocation = std::unique_ptr<VmaAllocation>(); 
	std::unordered_map<vk::Format, vk::ImageView> imageViews	= std::unordered_map<vk::Format, vk::ImageView>();				//image view: describe to vulkan how to access an image
	std::unique_ptr<vk::Sampler> textureSampler									= std::unique_ptr<vk::Sampler>();					//using sampler to apply filtering or other improvements over raw texel access

	virtual int getWidth() = 0;
	virtual int getHeight() = 0;
	virtual int getChannels() = 0;

	void createTextureImage(StarDevice& device);

	void transitionImageLayout(StarDevice& device, vk::Image image, vk::Format format, vk::ImageLayout oldLayout,
		vk::ImageLayout newLayout);

	void createImageSampler(StarDevice& device);

	vk::ImageView createImageView(StarDevice& device, vk::Image image, vk::Format format, vk::ImageAspectFlagBits aspectFlags);
};
}