#include "ScreenshotCommandBuffer.hpp"

star::ScreenshotBuffer::ScreenshotBuffer(StarDevice& device, std::vector<vk::Image>& swapChainImages, const vk::Extent2D& swapChainExtent, const vk::Format& swapChainImageFormat)
	: device(device), swapChainImages(swapChainImages), swapChainExtent(swapChainExtent), RenderResourceModifier()
{
	this->supportsBlit = deviceSupportsSwapchainBlit(swapChainImageFormat) && deviceSupportsBlitToLinearImage();
}

void star::ScreenshotBuffer::recordCommandBuffer(vk::CommandBuffer& commandBuffer, const int& frameInFlightIndex)
{
	vk::Image srcImage = this->swapChainImages[frameInFlightIndex];

	//transfer dstImage

	//create a barrier to prevent pipeline from moving forward until image transition is complete
	{
		vk::ImageMemoryBarrier barrier{};
		barrier.sType = vk::StructureType::eImageMemoryBarrier;     //specific flag for image operations
		barrier.oldLayout = vk::ImageLayout::eUndefined;
		barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;

		//if barrier is used for transferring ownership between queue families, this would be important -- set to ignore since we are not doing this
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		barrier.image = this->copyDstImages[frameInFlightIndex]->getImage();
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		barrier.subresourceRange.baseMipLevel = 0;                          //image does not have any mipmap levels
		barrier.subresourceRange.levelCount = 1;                            //image is not an array
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = {};
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, nullptr, barrier);
	}

	//transfer swapchain image
	{
		vk::ImageMemoryBarrier barrier{};
		barrier.sType = vk::StructureType::eImageMemoryBarrier;
		barrier.oldLayout = vk::ImageLayout::ePresentSrcKHR;
		barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
		barrier.image = srcImage;
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		barrier.subresourceRange.baseMipLevel = 0;                          //image does not have any mipmap levels
		barrier.subresourceRange.levelCount = 1;                            //image is not an array
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = vk::AccessFlagBits::eMemoryRead;
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

		commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eTransfer,
			{},
			{},
			nullptr,
			barrier);
	}

	if (this->supportsBlit) {
		//blit image
		vk::Offset3D blitSize = vk::Offset3D();
		blitSize.x = swapChainExtent.width;
		blitSize.y = swapChainExtent.height;
		blitSize.z = 1;

		vk::ImageBlit blit{};
		blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		blit.srcSubresource.layerCount = 1;
		blit.srcOffsets[1] = blitSize;

		blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		blit.dstSubresource.layerCount = 1;
		blit.dstOffsets[1] = blitSize;

		commandBuffer.blitImage(srcImage, vk::ImageLayout::eTransferSrcOptimal, this->copyDstImages[frameInFlightIndex]->getImage(),
			vk::ImageLayout::eTransferDstOptimal, 1, &blit, vk::Filter::eLinear);
	}
	else {
		//copy image
		vk::ImageCopy copyRegion{};
		copyRegion.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		copyRegion.srcSubresource.layerCount = 1;
		copyRegion.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		copyRegion.dstSubresource.layerCount = 1;
		copyRegion.extent.width = swapChainExtent.width;
		copyRegion.extent.height = swapChainExtent.height;
		copyRegion.extent.depth = 1;

		commandBuffer.copyImage(srcImage, vk::ImageLayout::eTransferSrcOptimal, this->copyDstImages[frameInFlightIndex]->getImage(), vk::ImageLayout::eTransferDstOptimal, 1, &copyRegion);
	}

	//need to transition images back to general layout for next frame use
	{
		//destination image
		vk::ImageMemoryBarrier barrier{};
		barrier.sType = vk::StructureType::eImageMemoryBarrier;
		barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.newLayout = vk::ImageLayout::eGeneral;
		barrier.image = this->copyDstImages[frameInFlightIndex]->getImage();
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		barrier.subresourceRange.baseMipLevel = 0;                          //image does not have any mipmap levels
		barrier.subresourceRange.levelCount = 1;                            //image is not an array
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eMemoryRead;

		commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eTransfer,
			{},
			{},
			nullptr,
			barrier);
	}

	{
		//source image
		vk::ImageMemoryBarrier barrier{};
		barrier.sType = vk::StructureType::eImageMemoryBarrier;
		barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
		barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
		barrier.image = srcImage;
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		barrier.subresourceRange.baseMipLevel = 0;                          //image does not have any mipmap levels
		barrier.subresourceRange.levelCount = 1;                            //image is not an array
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
		barrier.dstAccessMask = vk::AccessFlagBits::eMemoryRead;

		commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eTransfer,
			{},
			{},
			nullptr,
			barrier);
	}

	commandBuffer.end();
}

void star::ScreenshotBuffer::takeScreenshot(const std::string& path)
{
	this->submitMyBuffer(); 
	this->screenshotSavePath = std::string(path);
}

bool star::ScreenshotBuffer::deviceSupportsSwapchainBlit(const vk::Format& swapChainImageFormat)
{
	bool supportsBlit = true;

	vk::FormatProperties formatProperties = this->device.getPhysicalDevice().getFormatProperties(swapChainImageFormat);
	if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitSrc)) {
		supportsBlit = false;
	}

	return supportsBlit;
}

bool star::ScreenshotBuffer::deviceSupportsBlitToLinearImage()
{
	bool supportsBlit = true;

	vk::FormatProperties formatProperties = this->device.getPhysicalDevice().getFormatProperties(vk::Format::eR8G8B8A8Snorm);
	if (!(formatProperties.linearTilingFeatures & vk::FormatFeatureFlagBits::eBlitDst)) {
		supportsBlit = false;
	}

	return supportsBlit;
}

void star::ScreenshotBuffer::saveScreenshotToDisk(const int& bufferIndexJustRun)
{
	vk::ImageSubresource subResource{ vk::ImageAspectFlagBits::eColor, 0, 0 };
	vk::SubresourceLayout layout{};
	this->device.getDevice().getImageSubresourceLayout(this->copyDstImages[bufferIndexJustRun]->getImage(), &subResource, &layout);

	//give image to new thread for saving

	//create new image in slot


	//unsigned char* data = nullptr;
	//vmaMapMemory(this->device.getAllocator(), this->copyDstImageMemories[bufferIndexJustRun], (void**)&data);

	//{
	//	auto settings = StarImage::TextureCreateSettings(
	//		this->swapChainExtent.width, 
	//		this->swapChainExtent.height,
	//		4, 
	//		1,
	//		1,
	//		vk::ImageUsageFlagBits::eTransferDst,
	//		
	//	); 

	//}
	//FileTexture texture(this->swapChainExtent.width, this->swapChainExtent.height, 4, layout, data, true);
	// 
	
	//texture.saveToDisk(this->screenshotSavePath);
	//vmaUnmapMemory(this->device.getAllocator(), this->copyDstImageMemories[bufferIndexJustRun]);
}

void star::ScreenshotBuffer::initResources(StarDevice& device, const int& numFramesInFlight, const vk::Extent2D& screensize)
{
	this->copyDstImages.resize(numFramesInFlight);

	for (int i = 0; i < numFramesInFlight; i++) {
		this->copyDstImages[i] = createNewCopyToImage(device, this->swapChainExtent); 
		this->copyDstImages[i]->prepRender(device); 
	}
}

void star::ScreenshotBuffer::destroyResources(StarDevice& device)
{
	for (auto& image : this->copyDstImages) {
		image->cleanupRender(device); 
	}
}

star::Command_Buffer_Order star::ScreenshotBuffer::getCommandBufferOrder()
{
	return star::Command_Buffer_Order::end_of_frame;
}

star::Queue_Type star::ScreenshotBuffer::getCommandBufferType()
{
	if (this->supportsBlit)
		return star::Queue_Type::Tgraphics;
	else
		return star::Queue_Type::Ttransfer; 
}

vk::PipelineStageFlags star::ScreenshotBuffer::getWaitStages()
{
	return vk::PipelineStageFlagBits::eBottomOfPipe; 
}

bool star::ScreenshotBuffer::getWillBeSubmittedEachFrame()
{
	return false;
}

bool star::ScreenshotBuffer::getWillBeRecordedOnce()
{
	return true;
}

std::unique_ptr<star::StarImage> star::ScreenshotBuffer::createNewCopyToImage(star::StarDevice& device, const vk::Extent2D& screensize)
{
// 	return std::make_unique<StarImage>(StarTexture::TextureCreateSettings{
// 		static_cast<int>(screensize.width),
// 		static_cast<int>(screensize.height),
// 		4,
// 		1,
// 		1,
// 		vk::ImageUsageFlagBits::eTransferDst,
// 		vk::Format::eB8G8R8A8Srgb,
// 		vk::ImageAspectFlagBits::eColor,
// 		VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_TO_CPU,
// 		VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT,
// 		vk::ImageLayout::eTransferDstOptimal,
// 		false,
// 		false, 
// 		{},
// 		1.0f,
// 		vk::Filter::eNearest,
// 		"screenshotTargetTexture",
// 	});

	StarTexture::TextureCreateSettings imSetting = StarTexture::TextureCreateSettings(); 
	imSetting.width = static_cast<int>(screensize.width); 
	imSetting.height = static_cast<int>(screensize.height); 
	imSetting.channels = 4; 
	imSetting.byteDepth = 1;
	imSetting.depth = 1;
	imSetting.baseFormat = vk::Format::eB8G8R8A8Srgb;
	imSetting.usage = vk::ImageUsageFlagBits::eTransferDst;
	imSetting.memoryUsage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_TO_CPU; 

	return std::make_unique<StarImage>(imSetting); 

}
