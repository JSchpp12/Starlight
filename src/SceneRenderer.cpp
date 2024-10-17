#include "SceneRenderer.hpp"

namespace star {

SceneRenderer::SceneRenderer(star::StarScene& scene)
	: StarRenderer(*scene.getCamera()), scene(scene)
{
}

void SceneRenderer::prepare(StarDevice& device, const vk::Extent2D& swapChainExtent, const int& numFramesInFlight)
{
	this->swapChainExtent = std::make_unique<vk::Extent2D>(swapChainExtent);

	manualCreateDescriptors(device, numFramesInFlight);
	vk::Format depthFormat = createDepthResources(device, swapChainExtent, numFramesInFlight);

	this->renderToTargetInfo = std::make_unique<RenderingTargetInfo>(
		std::vector<vk::Format>{ this->getCurrentRenderToImageFormat()},
		depthFormat);

	this->renderToImages = createRenderToImages(device, numFramesInFlight);
	assert(this->renderToImages.size() > 0 && "Need at least 1 image for rendering"); 
	for (std::unique_ptr<Texture>& texture : this->renderToImages) {
		texture->prepRender(device); 
	}
	
	createRenderingGroups(device, swapChainExtent, numFramesInFlight);
}

std::vector<std::unique_ptr<Texture>> SceneRenderer::createRenderToImages(star::StarDevice& device, const int& numFramesInFlight)
{
	std::vector<std::unique_ptr<Texture>> newRenderToImages = std::vector<std::unique_ptr<Texture>>(); 

	auto imageCreateSettings = star::StarTexture::TextureCreateSettings::createDefault(false); 
	imageCreateSettings.imageFormat = this->getCurrentRenderToImageFormat();
	imageCreateSettings.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled;
	imageCreateSettings.allocationCreateFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT & VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;
	imageCreateSettings.memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	imageCreateSettings.imageFormat = this->getCurrentRenderToImageFormat();

	for (int i = 0; i < numFramesInFlight; i++) {
		newRenderToImages.push_back(std::make_unique<star::Texture>(this->swapChainExtent->width, this->swapChainExtent->height, imageCreateSettings));
		newRenderToImages.back()->prepRender(device); 

		auto oneTimeSetup = device.beginSingleTimeCommands(); 
		newRenderToImages.back()->transitionLayout(oneTimeSetup, vk::ImageLayout::eColorAttachmentOptimal, vk::AccessFlagBits::eNone, vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput);
		device.endSingleTimeCommands(oneTimeSetup); 
	}

	return newRenderToImages; 
}

void SceneRenderer::createRenderingGroups(StarDevice& device, const vk::Extent2D& swapChainExtent, const int& numFramesInFlight)
{	
	for (StarObject& object : this->scene.getObjects()) {
		//check if the object is compatible with any render groups 
		StarRenderGroup* match = nullptr; 

		//if it is not, create a new render group
		for (int i = 0; i < this->renderGroups.size(); i++) {
			if (this->renderGroups[i]->isObjectCompatible(object)) {
				match = this->renderGroups[i].get();
				break;
			}
		}

		if (match != nullptr) {
			match->addObject(object); 
		}
		else {
			//create a new one and add object
			this->renderGroups.push_back(std::unique_ptr<StarRenderGroup>(new StarRenderGroup(device, 
				numFramesInFlight, swapChainExtent, object)));
		}
	}

	//init all groups
	for (auto& group : this->renderGroups) {
		group->init(*globalSetLayout, this->globalDescriptorSets, this->getRenderingInfo());
	}
}

vk::ImageView SceneRenderer::createImageView(star::StarDevice& device, vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags)
{
	vk::ImageViewCreateInfo viewInfo{};
	viewInfo.sType = vk::StructureType::eImageViewCreateInfo;
	viewInfo.image = image;
	viewInfo.viewType = vk::ImageViewType::e2D;
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

void SceneRenderer::manualCreateDescriptors(star::StarDevice& device, const int& numFramesInFlight)
{
	this->globalSetLayout = StarDescriptorSetLayout::Builder(device)
		.addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eAll)
		.addBinding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eAll)
		.build();

	this->globalDescriptorSets.resize(numFramesInFlight);

	std::unique_ptr<std::vector<vk::DescriptorBufferInfo>> bufferInfos{};
	for (size_t i = 0; i < numFramesInFlight; i++) {
		//global
		bufferInfos = std::make_unique<std::vector<vk::DescriptorBufferInfo>>();
		

		auto globalBufferInfo = vk::DescriptorBufferInfo{
			ManagerBuffer::getBuffer(this->scene.getGlobalInfoBuffer(i)->getHandle()).getBuffer(),
			0,
			ManagerBuffer::getBuffer(this->scene.getGlobalInfoBuffer(i)->getHandle()).getBufferSize()};

		//buffer descriptors for point light locations 
		auto lightBufferInfo = vk::DescriptorBufferInfo{
			ManagerBuffer::getBuffer(this->scene.getLightInfoBuffer(i)->getHandle()).getBuffer(),
			0,
			ManagerBuffer::getBuffer(this->scene.getLightInfoBuffer(i)->getHandle()).getBufferSize() };

		this->globalDescriptorSets.at(i) = StarDescriptorWriter(device, *this->globalSetLayout, ManagerDescriptorPool::getPool())
			.writeBuffer(0, globalBufferInfo)
			.writeBuffer(1, lightBufferInfo)
			.build();
	}
}

void SceneRenderer::createImage(star::StarDevice& device, uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image, VmaAllocation& imageMemory)
{
	/* Create vulkan image */
	vk::ImageCreateInfo imageInfo{};
	imageInfo.sType = vk::StructureType::eImageCreateInfo;
	imageInfo.imageType = vk::ImageType::e2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageInfo.usage = usage;
	imageInfo.samples = vk::SampleCountFlagBits::e1;
	imageInfo.sharingMode = vk::SharingMode::eExclusive;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT & VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT; 
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	allocInfo.requiredFlags = (VkMemoryPropertyFlags)properties;

	vmaCreateImage(device.getAllocator(), (VkImageCreateInfo*)&imageInfo, &allocInfo, (VkImage*)&image, &imageMemory, nullptr);
}

vk::Format SceneRenderer::createDepthResources(star::StarDevice& device, const vk::Extent2D& swapChainExtent, const int& numFramesInFlight)
{
	//depth image should have:
	//  same resolution as the color attachment (in swap chain extent)
	//  optimal tiling and device local memory 
	//Need to decide format - need to decide format for the accuracy since no direct access to the depth image from CPU side 
	//Formats for color image: 
	//  VK_FORMAT_D32_SFLOAT: 32-bit-float
	//  VK_FORMAT_D32_SFLOAT_S8_UINT: 32-bit signed float for depth and 8 bit stencil component
	//  VK_FORMAT_D24_UNFORM_S8_UINT: 24-bit float for depth and 8 bit stencil component

	vk::Format depthFormat = findDepthFormat(device);

	this->renderToDepthImages.resize(numFramesInFlight); 
	this->renderToDepthImageMemory.resize(numFramesInFlight); 
	this->renderToDepthImageViews.resize(numFramesInFlight); 

	for (int i = 0; i < numFramesInFlight; i++) {

		createImage(device, swapChainExtent.width, swapChainExtent.height, depthFormat,
			vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment,
			vk::MemoryPropertyFlagBits::eDeviceLocal, this->renderToDepthImages[i], this->renderToDepthImageMemory[i]);

		this->renderToDepthImageViews[i] = createImageView(device, this->renderToDepthImages[i], depthFormat, vk::ImageAspectFlagBits::eDepth);
	}


	return depthFormat; 
}

vk::Format SceneRenderer::findDepthFormat(star::StarDevice& device)
{
	//utilizing the VK_FORMAT_FEATURE_ flag to check for candidates that have a depth component.
	return device.findSupportedFormat(
		{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
		vk::ImageTiling::eOptimal,
		vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

void SceneRenderer::initResources(StarDevice& device, const int& numFramesInFlight, const vk::Extent2D& screensize)
{
	this->prepare(device, screensize, numFramesInFlight);  
}

void SceneRenderer::destroyResources(StarDevice& device)
{
	for (auto& image : this->renderToImages) {
		image->cleanupRender(device);
	}
	for (vk::ImageView& imageView : this->renderToDepthImageViews) {
		device.getDevice().destroyImageView(imageView);
	}
	for (int i = 0; i < this->renderToDepthImages.size(); i++) {
		vmaDestroyImage(device.getAllocator(), this->renderToDepthImages[i], this->renderToDepthImageMemory[i]);
	}
}

std::vector<std::pair<vk::DescriptorType, const int>> SceneRenderer::getDescriptorRequests(const int& numFramesInFlight)
{
	return std::vector<std::pair<vk::DescriptorType, const int>>{
		std::pair<vk::DescriptorType, const int>(vk::DescriptorType::eUniformBuffer, numFramesInFlight),
		std::pair<vk::DescriptorType, const int>(vk::DescriptorType::eStorageBuffer, numFramesInFlight)
	};
}

void SceneRenderer::createDescriptors(star::StarDevice& device, const int& numFramesInFlight)
{
}

void SceneRenderer::recordCommandBuffer(vk::CommandBuffer& commandBuffer, const int& frameInFlightIndex)
{
	vk::Viewport viewport = this->prepareRenderingViewport();
	commandBuffer.setViewport(0, viewport);

	this->recordPreRenderingCalls(commandBuffer, frameInFlightIndex);

	{
		//dynamic rendering used...so dont need all that extra stuff
		vk::RenderingAttachmentInfo colorAttachmentInfo = prepareDynamicRenderingInfoColorAttachment(frameInFlightIndex);
		vk::RenderingAttachmentInfo depthAttachmentInfo = prepareDynamicRenderingInfoDepthAttachment(frameInFlightIndex);

		auto renderArea = vk::Rect2D{ vk::Offset2D{}, *this->swapChainExtent };
		vk::RenderingInfoKHR renderInfo{};
		renderInfo.renderArea = renderArea;
		renderInfo.layerCount = 1;
		renderInfo.pDepthAttachment = &depthAttachmentInfo;
		renderInfo.pColorAttachments = &colorAttachmentInfo;
		renderInfo.colorAttachmentCount = 1;
		commandBuffer.beginRendering(renderInfo);
	}

	this->recordRenderingCalls(commandBuffer, frameInFlightIndex);

	commandBuffer.endRendering();
}

vk::RenderingAttachmentInfo star::SceneRenderer::prepareDynamicRenderingInfoColorAttachment(const int& frameInFlightIndex) {
	vk::RenderingAttachmentInfoKHR colorAttachmentInfo{};
	colorAttachmentInfo.imageView = this->renderToImages[frameInFlightIndex]->getImageView();
	colorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	colorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachmentInfo.clearValue = vk::ClearValue{ vk::ClearValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}} };

	return colorAttachmentInfo;
}

vk::RenderingAttachmentInfo star::SceneRenderer::prepareDynamicRenderingInfoDepthAttachment(const int& frameInFlightIndex) {
	vk::RenderingAttachmentInfoKHR depthAttachmentInfo{};
	depthAttachmentInfo.imageView = this->renderToDepthImageViews[frameInFlightIndex];
	depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
	depthAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
	depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
	depthAttachmentInfo.clearValue = vk::ClearValue{ vk::ClearDepthStencilValue{1.0f} };

	return depthAttachmentInfo;
}

vk::Viewport SceneRenderer::prepareRenderingViewport()
{
	vk::Viewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)this->swapChainExtent->width;
	viewport.height = (float)this->swapChainExtent->height;
	//Specify values range of depth values to use for the framebuffer. If not doing anything special, leave at default
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	return viewport;
}

void SceneRenderer::recordPreRenderingCalls(vk::CommandBuffer& commandBuffer, const int& frameInFlightIndex)
{
	for (auto& group : this->renderGroups) {
		group->recordPreRenderPassCommands(commandBuffer, frameInFlightIndex);
	}
}

void SceneRenderer::recordRenderingCalls(vk::CommandBuffer& commandBuffer, const int& frameInFlightIndex)
{
	for (auto& group : this->renderGroups) {
		group->recordRenderPassCommands(commandBuffer, frameInFlightIndex);
	}
}

Command_Buffer_Type SceneRenderer::getCommandBufferType()
{
	return Command_Buffer_Type::Tgraphics;
}

void SceneRenderer::prepareForSubmission(const int& frameIndexToBeDrawn)
{
	for (StarObject& obj : this->scene.getObjects()) {
		obj.prepDraw(frameIndexToBeDrawn); 
	}

}

std::optional<std::function<void(const int&)>> SceneRenderer::getBeforeBufferSubmissionCallback()
{
	return std::optional<std::function<void(const int&)>>(std::function<void(const int&)>(std::bind(&SceneRenderer::prepareForSubmission, this, std::placeholders::_1)));
}

}