#include "SceneRenderer.hpp"

namespace star {

SceneRenderer::SceneRenderer(star::StarScene& scene)
	: StarRenderer(*scene.getCamera()), scene(scene)
{
}

void SceneRenderer::prepare(StarDevice& device, const vk::Extent2D& swapChainExtent, const int& numFramesInFlight)
{
	this->swapChainExtent = std::make_unique<vk::Extent2D>(swapChainExtent);

	auto globalBuilder = manualCreateDescriptors(device, numFramesInFlight);

	this->renderToTargetInfo = std::make_unique<RenderingTargetInfo>(
		std::vector<vk::Format>{ this->getCurrentRenderToImageFormat()},
		this->findDepthFormat(device));

	this->renderToImages = createRenderToImages(device, numFramesInFlight);
	assert(this->renderToImages.size() > 0 && "Need at least 1 image for rendering");
	this->renderToDepthImages = createRenderToDepthImages(device, numFramesInFlight);
	assert(this->renderToDepthImages.size() > 0 && "Need at least 1 depth image for rendering");

	//for (std::unique_ptr<StarTexture>& texture : this->renderToImages) {
	//	texture->prepRender(device); 
	//}
	
	createRenderingGroups(device, swapChainExtent, numFramesInFlight, globalBuilder);
}

std::vector<std::unique_ptr<StarImage>> SceneRenderer::createRenderToImages(star::StarDevice& device, const int& numFramesInFlight)
{
	std::vector<std::unique_ptr<StarImage>> newRenderToImages = std::vector<std::unique_ptr<StarImage>>();

	auto imageCreateSettings = star::StarImage::TextureCreateSettings{
		static_cast<int>(this->swapChainExtent->width),
		static_cast<int>(this->swapChainExtent->height),
		4,
		1,
		1,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
		this->getCurrentRenderToImageFormat(),
		vk::ImageAspectFlagBits::eColor | vk::ImageAspectFlagBits::eDepth,
		VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY,
		VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, 
		vk::ImageLayout::eColorAttachmentOptimal,
		false, false 
	};

	imageCreateSettings.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled; 

	for (int i = 0; i < numFramesInFlight; i++) {
		newRenderToImages.push_back(std::make_unique<star::StarImage>(imageCreateSettings));

		newRenderToImages.back()->prepRender(device);

		auto oneTimeSetup = device.beginSingleTimeCommands(); 
		newRenderToImages.back()->transitionLayout(oneTimeSetup, vk::ImageLayout::eColorAttachmentOptimal, vk::AccessFlagBits::eNone, vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput);
		device.endSingleTimeCommands(oneTimeSetup); 
	}

	return newRenderToImages; 
}

std::vector<std::unique_ptr<StarImage>> SceneRenderer::createRenderToDepthImages(StarDevice& device, const int& numFramesInFlight)
{
	std::vector<std::unique_ptr<StarImage>> newRenderToImages = std::vector<std::unique_ptr<StarImage>>();

	auto imageCreateSettings = star::StarImage::TextureCreateSettings{
		static_cast<int>(this->swapChainExtent->width),
		static_cast<int>(this->swapChainExtent->height),
		1,
		1,
		1,
		vk::ImageUsageFlagBits::eDepthStencilAttachment,
		this->findDepthFormat(device),
		vk::ImageAspectFlagBits::eDepth,
		VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
		VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		vk::ImageLayout::eDepthAttachmentOptimal,
		false, false
	};
	imageCreateSettings.imageUsage = vk::ImageUsageFlagBits::eDepthStencilAttachment;

	for (int i = 0; i < numFramesInFlight; i++) {
		newRenderToImages.push_back(std::make_unique<star::StarImage>(imageCreateSettings));

		newRenderToImages.back()->prepRender(device); 

		auto oneTimeSetup = device.beginSingleTimeCommands();

		vk::ImageMemoryBarrier barrier{};
		barrier.sType = vk::StructureType::eImageMemoryBarrier;
		barrier.oldLayout = vk::ImageLayout::eUndefined;
		barrier.newLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		barrier.image = newRenderToImages.back()->getImage();
		barrier.srcAccessMask = vk::AccessFlagBits::eNone;
		barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;

		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
		barrier.subresourceRange.baseMipLevel = 0;                          //image does not have any mipmap levels
		barrier.subresourceRange.levelCount = 1;                            //image is not an array
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		oneTimeSetup.pipelineBarrier(
			vk::PipelineStageFlagBits::eTopOfPipe,                        //which pipeline stages should occurr before barrier 
			vk::PipelineStageFlagBits::eLateFragmentTests,                   //pipeline stage in which operations will wait on the barrier 
			{},
			{},
			nullptr,
			barrier
		);

		device.endSingleTimeCommands(oneTimeSetup);
	}

	return newRenderToImages; 
}

void SceneRenderer::createRenderingGroups(StarDevice& device, const vk::Extent2D& swapChainExtent, const int& numFramesInFlight, star::StarShaderInfo::Builder builder)
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
		group->init(builder, this->getRenderingInfo());
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

star::StarShaderInfo::Builder SceneRenderer::manualCreateDescriptors(star::StarDevice& device, const int& numFramesInFlight)
{
	auto globalBuilder = StarShaderInfo::Builder(device, numFramesInFlight);
	
	this->globalSetLayout = StarDescriptorSetLayout::Builder(device)
		.addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eAll)
		.addBinding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eAll)
		.build();
	globalBuilder.addSetLayout(this->globalSetLayout); 

	for (size_t i = 0; i < numFramesInFlight; i++) {
		globalBuilder
			.startOnFrameIndex(i)
			.startSet()
			.add(this->scene.getGlobalInfoBuffer(i)->getHandle())
			.add(this->scene.getLightInfoBuffer(i)->getHandle());
	}

	return globalBuilder; 
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

	for (auto& image : this->renderToDepthImages) {
		image->cleanupRender(device);
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
	depthAttachmentInfo.imageView = this->renderToDepthImages[frameInFlightIndex]->getImageView();
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