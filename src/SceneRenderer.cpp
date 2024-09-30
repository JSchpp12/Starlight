#include "SceneRenderer.hpp"

namespace star {

SceneRenderer::SceneRenderer(std::vector<std::unique_ptr<Light>>& lightList, 
	std::vector<std::reference_wrapper<StarObject>> objectList, 
	StarCamera& camera)
	: StarRenderer(camera), lightList(lightList), objectList(objectList)
{
}

void SceneRenderer::prepare(StarDevice& device, const vk::Extent2D& swapChainExtent, const int& numFramesInFlight)
{
	this->swapChainExtent = std::make_unique<vk::Extent2D>(swapChainExtent);

	createRenderingBuffers(device, numFramesInFlight);
	createDescriptors(device, numFramesInFlight);
	vk::Format depthFormat = createDepthResources(device, swapChainExtent, numFramesInFlight);

	this->renderToTargetInfo = std::make_unique<RenderingTargetInfo>(
		std::vector<vk::Format>{ this->getCurrentRenderToImageFormat()},
		depthFormat);

	createRenderToImages(device, numFramesInFlight, this->renderToImages, this->renderToImageAllocations); 
	assert(this->renderToImages.size() > 0 && "Need at least 1 image for rendering"); 
	
	createImageViews(device, numFramesInFlight, this->getCurrentRenderToImageFormat()); 
	createRenderingGroups(device, swapChainExtent, numFramesInFlight);
}

void SceneRenderer::createRenderToImages(star::StarDevice& device, const int& numFramesInFlight, std::vector<vk::Image>& newRenderToImages, std::vector<VmaAllocation>& newRenderToImageAllocations)
{
	newRenderToImages.resize(numFramesInFlight);
	newRenderToImageAllocations.resize(numFramesInFlight);

	for (int i = 0; i < numFramesInFlight; i++) {
		createImage(device, this->swapChainExtent->width, this->swapChainExtent->height, this->getCurrentRenderToImageFormat(),
			vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
			vk::MemoryPropertyFlagBits::eDeviceLocal, newRenderToImages[i], newRenderToImageAllocations[i]);
	}
}

void SceneRenderer::createImageViews(star::StarDevice& device, const int& numImages, const vk::Format& imageFormat)
{
	this->renderToImageViews.resize(numImages); 

	//need to create an imageView for each of the images available
	for (int i = 0; i < numImages; i++) {
		this->renderToImageViews[i] = createImageView(device, this->renderToImages[i], imageFormat, vk::ImageAspectFlagBits::eColor);
	}
}

void SceneRenderer::createRenderingGroups(StarDevice& device, const vk::Extent2D& swapChainExtent, const int& numFramesInFlight)
{	
	for (StarObject& object : objectList) {
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

void SceneRenderer::updateUniformBuffer(uint32_t currentImage)
{
	//update global ubo 
	GlobalUniformBufferObject globalUbo;
	globalUbo.proj = this->camera.getProjectionMatrix();
	//glm designed for openGL where the Y coordinate of the flip coordinates is inverted. Fix this by flipping the sign on the scaling factor of the Y axis in the projection matrix.
	globalUbo.proj[1][1] *= -1;
	globalUbo.view = this->camera.getViewMatrix();
	globalUbo.inverseView = glm::inverse(this->camera.getViewMatrix());
	globalUbo.numLights = static_cast<uint32_t>(this->lightList.size());

	this->globalUniformBuffers[currentImage]->writeToBuffer(&globalUbo, sizeof(globalUbo));

	//update buffer for light positions
	std::vector<LightBufferObject> lightInformation(this->lightList.size());
	LightBufferObject newBufferObject{};

	//write buffer information
	for (size_t i = 0; i < this->lightList.size(); i++) {
		Light& currLight = *this->lightList.at(i);
		newBufferObject.position = glm::vec4{ currLight.getPosition(), 1.0f };
		newBufferObject.direction = currLight.direction;
		newBufferObject.ambient = currLight.getAmbient();
		newBufferObject.diffuse = currLight.getDiffuse();
		newBufferObject.specular = currLight.getSpecular();
		newBufferObject.settings.x = currLight.getEnabled() ? 1 : 0;
		newBufferObject.settings.y = currLight.getType();
		newBufferObject.controls.x = glm::cos(glm::radians(currLight.getInnerDiameter()));		//represent the diameter of light as the cos of the light (increase shader performance when doing comparison)
		newBufferObject.controls.y = glm::cos(glm::radians(currLight.getOuterDiameter()));
		lightInformation[i] = newBufferObject;
	}
	this->lightBuffers[currentImage]->writeToBuffer(lightInformation.data(), sizeof(LightBufferObject) * lightInformation.size());
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

void SceneRenderer::createDescriptors(star::StarDevice& device, const int& numFramesInFlight)
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
			this->globalUniformBuffers[i]->getBuffer(),
			0,
			sizeof(GlobalUniformBufferObject) };

		//buffer descriptors for point light locations 
		auto lightBufferInfo = vk::DescriptorBufferInfo{
			this->lightBuffers[i]->getBuffer(),
			0,
			sizeof(LightBufferObject) * this->lightList.size() };

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

void SceneRenderer::createRenderingBuffers(star::StarDevice& device, const int& numFramesInFlight)
{
	vk::DeviceSize globalBufferSize = sizeof(GlobalUniformBufferObject) * this->objectList.size();

	this->globalUniformBuffers.resize(numFramesInFlight);
	if (this->lightList.size() > 0) {
		this->lightBuffers.resize(numFramesInFlight);
	}

	for (size_t i = 0; i < numFramesInFlight; i++) {
		auto numOfObjects = this->objectList.size(); 
		this->globalUniformBuffers[i] = std::make_unique<StarBuffer>(device, this->objectList.size(), sizeof(GlobalUniformBufferObject),
			vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		this->globalUniformBuffers[i]->map();

		//create light buffers 
		if (this->lightList.size() > 0) {
			this->lightBuffers[i] = std::make_unique<StarBuffer>(device, this->lightList.size(), sizeof(LightBufferObject) * this->lightList.size(),
				vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
			this->lightBuffers[i]->map();
		}
	}
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
	for (vk::ImageView& imageView : this->renderToImageViews) {
		device.getDevice().destroyImageView(imageView); 
	}
	for (vk::ImageView& imageView : this->renderToDepthImageViews) {
		device.getDevice().destroyImageView(imageView);
	}
	for (int i = 0; i < this->renderToDepthImages.size(); i++) {
		vmaDestroyImage(device.getAllocator(), this->renderToDepthImages[i], this->renderToDepthImageMemory[i]);
	}
	for (int i = 0; i < this->renderToImages.size(); i++) {
		vmaDestroyImage(device.getAllocator(), this->renderToImages[i], this->renderToImageAllocations[i]);
	}
}

std::vector<std::pair<vk::DescriptorType, const int>> SceneRenderer::getDescriptorRequests(const int& numFramesInFlight)
{
	return std::vector<std::pair<vk::DescriptorType, const int>>{
		std::pair<vk::DescriptorType, const int>(vk::DescriptorType::eUniformBuffer, numFramesInFlight),
		std::pair<vk::DescriptorType, const int>(vk::DescriptorType::eStorageBuffer, numFramesInFlight)
	};
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
	colorAttachmentInfo.imageView = this->renderToImageViews[frameInFlightIndex];
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
	for (StarObject& obj : this->objectList) {
		obj.prepDraw(frameIndexToBeDrawn); 
	}

	updateUniformBuffer(frameIndexToBeDrawn);
}

std::optional<std::function<void(const int&)>> SceneRenderer::getBeforeBufferSubmissionCallback()
{
	return std::optional<std::function<void(const int&)>>(std::function<void(const int&)>(std::bind(&SceneRenderer::prepareForSubmission, this, std::placeholders::_1)));
}

}