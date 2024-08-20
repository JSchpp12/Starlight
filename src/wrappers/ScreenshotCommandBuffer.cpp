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

		barrier.image = this->copyDstImages[frameInFlightIndex];
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
		VkOffset3D blitSize;
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

		commandBuffer.blitImage(srcImage, vk::ImageLayout::eTransferSrcOptimal, this->copyDstImages[frameInFlightIndex],
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

		commandBuffer.copyImage(srcImage, vk::ImageLayout::eTransferSrcOptimal, this->copyDstImages[frameInFlightIndex], vk::ImageLayout::eTransferDstOptimal, 1, &copyRegion);
	}

	//need to transition images back to general layout for next frame use
	{
		//destination image
		vk::ImageMemoryBarrier barrier{};
		barrier.sType = vk::StructureType::eImageMemoryBarrier;
		barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.newLayout = vk::ImageLayout::eGeneral;
		barrier.image = this->copyDstImages[frameInFlightIndex];
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

std::optional<std::function<void(const int&)>> star::ScreenshotBuffer::getAfterBufferSubmissionCallback()
{
	return std::optional<std::function<void(const int&)>>(std::bind(&ScreenshotBuffer::saveScreenshotToDisk, this, std::placeholders::_1));
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
	this->device.getDevice().getImageSubresourceLayout(this->copyDstImages[bufferIndexJustRun], &subResource, &layout);

	unsigned char* data = nullptr;
	vmaMapMemory(this->device.getAllocator(), this->copyDstImageMemories[bufferIndexJustRun], (void**)&data);
	Texture texture(this->swapChainExtent.width, this->swapChainExtent.height, 4, layout, data, true);
	texture.saveToDisk(this->screenshotSavePath);
	vmaUnmapMemory(this->device.getAllocator(), this->copyDstImageMemories[bufferIndexJustRun]);
}

void star::ScreenshotBuffer::initResources(StarDevice& device, const int& numFramesInFlight)
{
	this->copyDstImages.resize(numFramesInFlight);
	this->copyDstImageMemories.resize(numFramesInFlight);
	for (int i = 0; i < numFramesInFlight; i++) {
		vk::ImageCreateInfo imageInfo{};
		imageInfo.sType = vk::StructureType::eImageCreateInfo;
		imageInfo.imageType = vk::ImageType::e2D;
		imageInfo.extent.width = this->swapChainExtent.width;
		imageInfo.extent.height = this->swapChainExtent.height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = vk::Format::eB8G8R8A8Srgb;
		imageInfo.tiling = vk::ImageTiling::eLinear;
		imageInfo.initialLayout = vk::ImageLayout::eUndefined;
		imageInfo.usage = vk::ImageUsageFlagBits::eTransferDst;
		imageInfo.samples = vk::SampleCountFlagBits::e1;
		imageInfo.sharingMode = vk::SharingMode::eExclusive;


		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
		allocInfo.requiredFlags = (VkMemoryPropertyFlags)vk::MemoryPropertyFlagBits::eHostVisible;

		vmaCreateImage(this->device.getAllocator(), (VkImageCreateInfo*)&imageInfo, &allocInfo, (VkImage*)&copyDstImages[i], &copyDstImageMemories[i], nullptr);
	}
}

void star::ScreenshotBuffer::destroyResources(StarDevice& device)
{
	for (int i = 0; i < this->copyDstImageMemories.size(); i++) {
		vmaDestroyImage(this->device.getAllocator(), this->copyDstImages[i], this->copyDstImageMemories[i]);
	}
}

star::Command_Buffer_Order star::ScreenshotBuffer::getCommandBufferOrder()
{
	return star::Command_Buffer_Order::end_of_frame;
}

star::Command_Buffer_Type star::ScreenshotBuffer::getCommandBufferType()
{
	if (this->supportsBlit)
		return star::Command_Buffer_Type::Tgraphics;
	else
		return star::Command_Buffer_Type::Ttransfer; 
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
