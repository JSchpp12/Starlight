#include "StarImage.hpp"


#include "StarDescriptorBuilders.hpp"
#include "ConfigFile.hpp"

#include <sstream>
#include <optional>

namespace star {

void StarImage::prepRender(StarDevice& device) {
	if (!this->textureImage)
	{
		this->StarTexture::prepRender(device);
		createImage(device);
	}

	createTextureImageView(device, this->createSettings.imageFormat, this->createSettings.aspectFlags);
	if (this->createSettings.createSampler)
		createImageSampler(device);
}

void StarImage::cleanupRender(StarDevice& device)
{
	if (this->textureSampler)
		device.getDevice().destroySampler(*this->textureSampler);

	for (auto& item : this->imageViews) {
		device.getDevice().destroyImageView(item.second);
	}


}

vk::ImageView StarImage::getImageView(vk::Format* requestedFormat) const{
	if (requestedFormat != nullptr) {
		//make sure the image view actually exists for the requested format
		assert(this->imageViews.find(*requestedFormat) != this->imageViews.end() && "The image must be created with the proper image view before being requested");
		return this->imageViews.at(*requestedFormat);
	}

	//just grab the first one
	for (auto& view : this->imageViews) {
		return view.second; 
	}
}

void StarImage::createImage(StarDevice& device) { 
	auto loadedData = this->loadImageData(device); 
	if (loadedData) {
		//copy staging buffer to texture image 
		transitionImageLayout(device, this->textureImage, this->createSettings.imageFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

		device.copyBufferToImage(loadedData->getVulkanBuffer(), this->textureImage, static_cast<uint32_t>(this->createSettings.width), static_cast<uint32_t>(this->createSettings.height));

		//prepare final image for texture mapping in shaders 
		transitionImageLayout(device, this->textureImage, this->createSettings.imageFormat, vk::ImageLayout::eTransferDstOptimal, this->createSettings.initialLayout);
	}
}

void StarImage::transitionLayout(vk::CommandBuffer& commandBuffer, 
	vk::ImageLayout newLayout, vk::AccessFlags srcFlags, vk::AccessFlags dstFlags, 
	vk::PipelineStageFlags sourceStage, vk::PipelineStageFlags dstStage)
{
	vk::ImageMemoryBarrier barrier{};
	barrier.sType = vk::StructureType::eImageMemoryBarrier;
	barrier.oldLayout = this->layout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = this->textureImage;
	barrier.srcAccessMask = srcFlags;
	barrier.dstAccessMask = dstFlags;

	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.baseMipLevel = 0;                          //image does not have any mipmap levels
	barrier.subresourceRange.levelCount = 1;                            //image is not an array
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	commandBuffer.pipelineBarrier(
		sourceStage,                        //which pipeline stages should occurr before barrier 
		dstStage,                   //pipeline stage in which operations will wait on the barrier 
		{},
		{},
		nullptr,
		barrier
	);

	this->layout = newLayout; 
}

void StarImage::transitionImageLayout(StarDevice& device, vk::Image image, vk::Format format, vk::ImageLayout oldLayout,
	vk::ImageLayout newLayout) {
	vk::CommandBuffer commandBuffer = device.beginSingleTimeCommands();

	//create a barrier to prevent pipeline from moving forward until image transition is complete
	vk::ImageMemoryBarrier barrier{};
	barrier.sType = vk::StructureType::eImageMemoryBarrier;     //specific flag for image operations
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;

	//if barrier is used for transferring ownership between queue families, this would be important -- set to ignore since we are not doing this
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;
	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.baseMipLevel = 0;                          //image does not have any mipmap levels
	barrier.subresourceRange.levelCount = 1;                            //image is not an array
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	//the operations that need to be completed before and after the barrier, need to be defined
	barrier.srcAccessMask = {}; //TODO
	barrier.dstAccessMask = {}; //TODO

	vk::PipelineStageFlags sourceStage, destinationStage;

	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
		//undefined transition state, dont need to wait for this to complete
		barrier.srcAccessMask = {};
		//barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && (newLayout == vk::ImageLayout::eShaderReadOnlyOptimal || newLayout == vk::ImageLayout::eGeneral)) {
		//transfer destination shader reading, will need to wait for completion. Especially in the frag shader where reads will happen
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else if (oldLayout == vk::ImageLayout::eShaderReadOnlyOptimal && newLayout == vk::ImageLayout::eTransferDstOptimal) {
		//preparing to update texture during runtime, need to wait for top of pipe
		//barrier.srcAccessMask = vk::AccessFlagBits::;
		barrier.srcAccessMask = {}; 
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead; 

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else{
		throw std::invalid_argument("unsupported layout transition!");
	}

	//transfer writes must occurr during the pipeline transfer stage
	commandBuffer.pipelineBarrier(
		sourceStage,                        //which pipeline stages should occurr before barrier 
		destinationStage,                   //pipeline stage in which operations will wait on the barrier 
		{},
		{},
		nullptr,
		barrier
	);

	device.endSingleTimeCommands(commandBuffer);
}

void StarImage::createImageSampler(StarDevice& device) {
	//get device properties for amount of anisotropy permitted
	vk::PhysicalDeviceProperties deviceProperties = device.getPhysicalDevice().getProperties();

	vk::SamplerCreateInfo samplerInfo{};
	samplerInfo.sType = vk::StructureType::eSamplerCreateInfo;

	//anisotropy level
	{
		float anisotropyLevel = 1.0;
		{
			auto anisotropySetting = ConfigFile::getSetting(Config_Settings::texture_anisotropy);
			if (anisotropySetting == "max") {
				anisotropyLevel = deviceProperties.limits.maxSamplerAnisotropy;
			}
			else {
				try {
					anisotropyLevel = std::stof(anisotropySetting);
					if (anisotropyLevel < 1.0f) {
						throw std::runtime_error("Anisotropy level must be greater than 1.0");
					}
				}
				catch (std::exception) {
					throw std::runtime_error("Anisotropy setting must be a float or 'max'");
				}
			}
		}
		//should anisotropic filtering be used? Really only matters if performance is a concern
		samplerInfo.anisotropyEnable = VK_TRUE;
		//specifies the limit on the number of texel samples that can be used (lower = better performance)
		samplerInfo.maxAnisotropy = anisotropyLevel;
	}

	//texture filtering
	{
		vk::Filter filterType = vk::Filter::eNearest;
		{

			auto textureFilteringSetting = ConfigFile::getSetting(Config_Settings::texture_filtering);
			if (textureFilteringSetting == "nearest") {
				filterType = vk::Filter::eNearest;
			}
			else if (textureFilteringSetting == "linear") {
				filterType = vk::Filter::eLinear;
			}
			else {
				throw std::runtime_error("Texture filtering setting must be 'nearest' or 'linear'");
			}
		}

		//how to sample textures that are magnified 
		samplerInfo.magFilter = filterType;
		//how to sample textures that are minified
		samplerInfo.minFilter = filterType;
	}

	//repeat mode - repeat the texture when going beyond the image dimensions
	samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;

	samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
	//specifies coordinate system to use in addressing texels. 
		//VK_TRUE - use coordinates [0, texWidth) and [0, texHeight]
		//VK_FALSE - use [0, 1)
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	//if comparing, the texels will first compare to a value, the result of the comparison is used in filtering operations (percentage-closer filtering on shadow maps)
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = vk::CompareOp::eAlways;

	//following apply to mipmapping -- not using here
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	this->textureSampler = std::make_unique<vk::Sampler>(device.getDevice().createSampler(samplerInfo));
	if (!this->textureSampler) {
		throw std::runtime_error("failed to create texture sampler!");
	}
}

void StarImage::createTextureImageView(StarDevice& device, const vk::Format& viewFormat, const vk::ImageAspectFlags& aspectFlags) {
	vk::ImageView imageView = createImageView(device, this->textureImage, viewFormat, aspectFlags);
	this->imageViews.insert(std::pair<vk::Format, vk::ImageView>(viewFormat, imageView)); 
}

vk::ImageView StarImage::createImageView(StarDevice& device, vk::Image image, 
	vk::Format format, const vk::ImageAspectFlags& aspectFlags) {
	vk::ImageViewCreateInfo viewInfo{};
	viewInfo.sType = vk::StructureType::eImageViewCreateInfo;
	viewInfo.image = image;
	if (this->getDepth() > 1)
		viewInfo.viewType = vk::ImageViewType::e3D;
	else {
		viewInfo.viewType = vk::ImageViewType::e2D;
	}
	viewInfo.format = format;

	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	vk::ImageView imageView = device.getDevice().createImageView(viewInfo);

	if (!imageView) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}
}