#include "StarImage.hpp"


#include "StarDescriptorBuilders.hpp"
#include "ConfigFile.hpp"

#include <sstream>
#include <optional>

namespace star {

void StarImage::prepRender(StarDevice& device) {
	if (!this->texture)
	{
		this->texture = std::make_unique<StarTexture>(this->createSettings, device.getDevice(), device.getAllocator().get());
		createImage(device);
	}
}

void StarImage::cleanupRender(StarDevice& device)
{
	this->texture.release();
}

void StarImage::createImage(StarDevice& device) { 
	auto loadedData = this->loadImageData(device); 
	if (loadedData) {
		//copy staging buffer to texture image 
		transitionImageLayout(device, this->texture->getImage(), this->createSettings.imageFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

		device.copyBufferToImage(loadedData->getVulkanBuffer(), this->texture->getImage(), static_cast<uint32_t>(this->createSettings.width), static_cast<uint32_t>(this->createSettings.height));

		//prepare final image for texture mapping in shaders 
		transitionImageLayout(device, this->texture->getImage(), this->createSettings.imageFormat, vk::ImageLayout::eTransferDstOptimal, this->createSettings.initialLayout);
	}
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

}