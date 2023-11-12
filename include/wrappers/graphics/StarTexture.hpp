//wrapper class for textures 
#pragma once

#include "StarDevice.hpp"
#include "StarBuffer.hpp"
#include "StarDescriptors.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>

namespace star {
class StarTexture {
public:
	/// <summary>
	/// Options to be used when creating a new texture
	/// </summary>
	/// <returns></returns>
	struct TextureCreateSettings {
		TextureCreateSettings() = default; 

		TextureCreateSettings(vk::Flags<vk::ImageUsageFlagBits> imageUsage, vk::Format imageFormat)
			: imageUsage(imageUsage), imageFormat(imageFormat) {}
			~TextureCreateSettings() = default; 

		vk::Flags<vk::ImageUsageFlagBits> imageUsage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
		vk::Format imageFormat = vk::Format::eR8G8B8A8Srgb;
	}; 

	StarTexture() : createSettings(std::make_unique<TextureCreateSettings>()) {}; 
	StarTexture(TextureCreateSettings& settings)
		: createSettings(std::make_unique<TextureCreateSettings>(settings)) {}; 

	virtual ~StarTexture();

	virtual void prepRender(StarDevice& device);

	void cleanupRender(StarDevice& device); 

	/// <summary>
	/// Read the image from disk into memory or provide the image which is in memory
	/// </summary>
	/// <returns></returns>
	virtual std::unique_ptr<unsigned char> data() = 0;

	vk::ImageView getImageView() { return this->textureImageView; }
	vk::Sampler getSampler() { return this->textureSampler; }

protected:
	std::unique_ptr<TextureCreateSettings> createSettings = nullptr; 

	bool isRenderReady = false; 
	vk::Image textureImage;
	vk::ImageView textureImageView;				//image view: describe to vulkan how to access an image
	vk::Sampler textureSampler;					//using sampler to apply filtering or other improvements over raw texel access
	vk::DeviceMemory imageMemory;				//device memory where image will be stored
	vk::DescriptorSet descriptorSet;

	virtual int getWidth() = 0;
	virtual int getHeight() = 0;
	virtual int getChannels() = 0;

	void createTextureImage(StarDevice& device);

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
	void createImage(StarDevice& device, uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
		vk::ImageUsageFlags usage, vk::MemoryPropertyFlagBits properties,
		vk::Image& image, vk::DeviceMemory& imageMemory);

	void transitionImageLayout(StarDevice& device, vk::Image image, vk::Format format, vk::ImageLayout oldLayout,
		vk::ImageLayout newLayout);

	void createImageSampler(StarDevice& device);

	void createTextureImageView(StarDevice& device);

	vk::ImageView createImageView(StarDevice& device, vk::Image image, vk::Format format, vk::ImageAspectFlagBits aspectFlags);
};
}