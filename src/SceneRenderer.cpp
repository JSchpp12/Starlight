#include "SceneRenderer.hpp"

typedef std::chrono::high_resolution_clock Clock;

namespace star {

SceneRenderer::SceneRenderer(StarWindow& window, std::vector<std::unique_ptr<Light>>& lightList, 
	std::vector<std::reference_wrapper<StarObject>> objectList, 
	StarCamera& camera, StarDevice& device)
	: StarRenderer(window, camera, device), lightList(lightList), objectList(objectList){
}

void SceneRenderer::prepare(const vk::Extent2D& swapChainExtent, const int& numFramesInFlight, const vk::Format& resultingRenderImageFormat)
{
	this->swapChainExtent = std::make_unique<vk::Extent2D>(swapChainExtent);

	createRenderingBuffers(numFramesInFlight);
	createDescriptors(numFramesInFlight);
	createDepthResources(swapChainExtent);

	this->renderToImages = createRenderToImages(); 
	assert(this->renderToImages.size() != 0 && "The function or overriden function must create the renderToImages variable"); 
	
	createImageViews(numFramesInFlight, resultingRenderImageFormat); 
	//createRenderPass(swapChainExtent, resultingRenderImageFormat);
	//createFramebuffers(numFramesInFlight); 
	createRenderingGroups(swapChainExtent, numFramesInFlight);
	createCommandBuffers();
}

void SceneRenderer::createFramebuffers(const int& numFramesInFlight)
{
	/*this->renderToFramebuffers.resize(numFramesInFlight);

	for (int i = 0; i < numFramesInFlight; i++) {
		std::vector<vk::ImageView> attachments = {
			this->renderToImageViews[i],
			depthImageView
		};

		vk::FramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = vk::StructureType::eFramebufferCreateInfo;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		this->renderToFramebuffers[i] = this->device.getDevice().createFramebuffer(framebufferInfo);
		if (!this->renderToFramebuffers[i]) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}*/

	
}

std::vector<vk::Image> SceneRenderer::createRenderToImages()
{
	return std::vector<vk::Image>();
}

void SceneRenderer::createImageViews(const int& numFramesInFlight, const vk::Format& renderToImageFormat)
{
	this->renderToImageViews.resize(numFramesInFlight);

	//need to create an imageView for each of the images available
	for (int i = 0; i < numFramesInFlight; i++) {
		this->renderToImageViews[i] = createImageView(this->renderToImages[i], renderToImageFormat, vk::ImageAspectFlagBits::eColor);
	}
}

void SceneRenderer::createRenderingGroups(const vk::Extent2D& swapChainExtent, const int& numFramesInFlight)
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
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

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

vk::ImageView SceneRenderer::createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlagBits aspectFlags)
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

	vk::ImageView imageView = this->device.getDevice().createImageView(viewInfo);

	if (!imageView) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

void SceneRenderer::createDescriptors(const int& numFramesInFlight)
{
	this->globalSetLayout = StarDescriptorSetLayout::Builder(this->device)
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

		this->globalDescriptorSets.at(i) = StarDescriptorWriter(this->device, *this->globalSetLayout, ManagerDescriptorPool::getPool())
			.writeBuffer(0, globalBufferInfo)
			.writeBuffer(1, lightBufferInfo)
			.build();
	}
}

void SceneRenderer::createRenderPass(const vk::Extent2D& swapChainExtent, const vk::Format& swapChainFormat)
{
	/*  Single render pass consists of many small subpasses
		each subpasses are subsequent rendering operations that depend on the contents of framebuffers in the previous pass.
		It is best to group these into one rendering pass, then vulkan can optimize for this in order to save memory bandwidth.
		For this program, we are going to stick with one subpass
	*/
	/* Depth attachment */
	vk::AttachmentDescription depthAttachment{};
	depthAttachment.format = findDepthFormat();
	depthAttachment.samples = vk::SampleCountFlagBits::e1;
	depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;     //wont be used after draw 
	depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	depthAttachment.initialLayout = vk::ImageLayout::eUndefined;    //dont care about previous depth contents 
	depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::AttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	/* Color attachment */
	vk::AttachmentDescription colorAttachment{};
	//format of color attachment needs to match the swapChain image format
	colorAttachment.format = swapChainFormat;
	//no multisampling needed so leave at 1 samples
	colorAttachment.samples = vk::SampleCountFlagBits::e1;
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;

	/* Choices for loadOp:
		1. VK_ATTACHMENT_LOAD_OP_LOAD: preserve the existing contents of the attachment
		2. VK_ATTACHMENT_LOAD_OP_CLEAR: clear the values to a constant at the start
		3. VK_ATTACHMENT_LOAD_OP_DONT_CARE: existing contents are undefined
	*/
	//what do to with data before rendering
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	/* Choices for storeOp:
		1. VK_ATTACHMENT_STORE_OP_STORE: rendered contents will be stored for later use
		2. VK_ATTACHMENT_STORE_OP_DONT_CARE: contents of the frame buffer will be undefined after the rendering operation
	*/
	//what to do with data after rendering
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;    //since we want to see the rendered triangle, we are going to store 

	/*Image layouts can change depending on what operation is being performed
	* Possible layouts are:
	* 1. VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: image is used as a color attachment
	* 2. VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: images to be presented in the swap chain
	* 3. VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: images to be used as destination for memory copy operation
	*/
	//dont care what format image is in before render - contents of image are not guaranteed to be preserved 
	colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
	//want image to be ready for display 
	colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

	/* Color attachment references */
	vk::AttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;    //will give best performance

	/* Subpass */
	vk::SubpassDescription subpass{};
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	/* Subpass Dependencies */
	vk::SubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
	dependency.srcAccessMask = vk::AccessFlagBits::eNone;
	dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
	dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite; //allow for write 

	/* Render Pass */
	std::array<vk::AttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
	vk::RenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = vk::StructureType::eRenderPassCreateInfo;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	this->renderPass = this->device.getDevice().createRenderPass(renderPassInfo);
	if (!renderPass) {
		throw std::runtime_error("failed to create render pass");
	}
}

void SceneRenderer::createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image, VmaAllocation& imageMemory)
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

	vmaCreateImage(this->device.getAllocator(), (VkImageCreateInfo*)&imageInfo, &allocInfo, (VkImage*)&image, &imageMemory, nullptr);
}

void SceneRenderer::createRenderingBuffers(const int& numFramesInFlight)
{
	vk::DeviceSize globalBufferSize = sizeof(GlobalUniformBufferObject) * this->objectList.size();

	this->globalUniformBuffers.resize(numFramesInFlight);
	if (this->lightList.size() > 0) {
		this->lightBuffers.resize(numFramesInFlight);
	}

	for (size_t i = 0; i < numFramesInFlight; i++) {
		auto numOfObjects = this->objectList.size(); 
		this->globalUniformBuffers[i] = std::make_unique<StarBuffer>(this->device, this->objectList.size(), sizeof(GlobalUniformBufferObject),
			vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		this->globalUniformBuffers[i]->map();

		//create light buffers 
		if (this->lightList.size() > 0) {
			this->lightBuffers[i] = std::make_unique<StarBuffer>(this->device, this->lightList.size(), sizeof(LightBufferObject) * this->lightList.size(),
				vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
			this->lightBuffers[i]->map();
		}
	}
}

void SceneRenderer::createCommandBuffers()
{
	//this->screenshotCommandBuffer = std::make_unique<ScreenshotBuffer>(device, this->swapChainImages, this->swapChainExtent, this->swapChainImageFormat);
}

void SceneRenderer::createDepthResources(const vk::Extent2D& swapChainExtent)
{
	//depth image should have:
	//  same resolution as the color attachment (in swap chain extent)
	//  optimal tiling and device local memory 
	//Need to decide format - need to decide format for the accuracy since no direct access to the depth image from CPU side 
	//Formats for color image: 
	//  VK_FORMAT_D32_SFLOAT: 32-bit-float
	//  VK_FORMAT_D32_SFLOAT_S8_UINT: 32-bit signed float for depth and 8 bit stencil component
	//  VK_FORMAT_D24_UNFORM_S8_UINT: 24-bit float for depth and 8 bit stencil component

	vk::Format depthFormat = findDepthFormat();

	createImage(swapChainExtent.width, swapChainExtent.height, depthFormat,
		vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment,
		vk::MemoryPropertyFlagBits::eDeviceLocal, depthImage, depthImageMemory);

	this->depthImageView = createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);
}

vk::SurfaceFormatKHR SceneRenderer::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats) {
		//check if a format allows 8 bits for R,G,B, and alpha channel
		//use SRGB color space

		if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			return availableFormat;
		}
	}

	//if nothing matches what we are looking for, just take what is available
	return availableFormats[0];
}

vk::PresentModeKHR SceneRenderer::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
	/*
	* There are a number of swap modes that are in vulkan
	* 1. VK_PRESENT_MODE_IMMEDIATE_KHR: images submitted by application are sent to the screen right away -- can cause tearing
	* 2. VK_PRESENT_MODE_FIFO_RELAXED_KHR: images are placed in a queue and images are sent to the display in time with display refresh (VSYNC like). If queue is full, application has to wait
	*   "Vertical blank" -> time when the display is refreshed
	* 3. VK_PRESENT_MODE_FIFO_RELAXED_KHR: same as above. Except if the application is late, and the queue is empty: the next image submitted is sent to display right away instead of waiting for next blank.
	* 4. VK_PRESENT_MODE_MAILBOX_KHR: similar to #2 option. Instead of blocking applicaiton when the queue is full, the images in the queue are replaced with newer images.
	*   This mode can be used to render frames as fast as possible while still avoiding tearing. Kind of like "tripple buffering". Does not mean that framerate is unlocked however.
	*   Author of tutorial statement: this mode [4] is a good tradeoff if energy use is not a concern. On mobile devices it might be better to go with [2]
	*/

	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
			return availablePresentMode;
		}
	}

	//only VK_PRESENT_MODE_FIFO_KHR is guaranteed to be available
	return vk::PresentModeKHR::eFifo;
}

vk::Extent2D SceneRenderer::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
{
	/*
	* "swap extent" -> resolution of the swap chain images (usually the same as window resultion
	* Range of available resolutions are defined in VkSurfaceCapabilitiesKHR
	* Resolution can be changed by setting value in currentExtent to the maximum value of a uint32_t
	*   then: the resolution can be picked by matching window size in minImageExtent and maxImageExtent
	*/
	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	}
	else {
		//vulkan requires that resultion be defined in pixels -- if a high DPI display is used, screen coordinates do not match with pixels
		int width, height;
		glfwGetFramebufferSize(this->window.getGLFWwindow(), &width, &height);

		vk::Extent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		//(clamp) -- keep the width and height bounded by the permitted resolutions 
		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

vk::Format SceneRenderer::findDepthFormat()
{
	//utilizing the VK_FORMAT_FEATURE_ flag to check for candidates that have a depth component.
	return this->device.findSupportedFormat(
		{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
		vk::ImageTiling::eOptimal,
		vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

void SceneRenderer::initResources(StarDevice& device, const int& numFramesInFlight)
{
	ManagerDescriptorPool::request(vk::DescriptorType::eUniformBuffer, numFramesInFlight);
	ManagerDescriptorPool::request(vk::DescriptorType::eStorageBuffer, numFramesInFlight);
}

void SceneRenderer::destroyResources(StarDevice& device)
{

}

void SceneRenderer::recordCommandBuffer(vk::CommandBuffer& commandBuffer, const int& frameInFlightIndex)
{
	//vk::Viewport viewport{};
	//viewport.x = 0.0f;
	//viewport.y = 0.0f;
	//viewport.width = (float)this->swapChainExtent.width;
	//viewport.height = (float)this->swapChainExtent.height;
	////Specify values range of depth values to use for the framebuffer. If not doing anything special, leave at default
	//viewport.minDepth = 0.0f;
	//viewport.maxDepth = 1.0f;

	//commandBuffer.setViewport(0, viewport);

	//for (auto& group : this->renderGroups) {
	//	group->recordPreRenderPassCommands(commandBuffer, frameInFlightIndex);
	//}

	///* Begin render pass */
	////drawing starts by beginning a render pass 
	//vk::RenderPassBeginInfo renderPassInfo{};
	//renderPassInfo.sType = vk::StructureType::eRenderPassBeginInfo;

	////define the render pass we want
	//renderPassInfo.renderPass = renderPass;

	////what attachments do we need to bind
	////previously created swapChainbuffers to hold this information 
	//renderPassInfo.framebuffer = swapChainFramebuffers[this->currentSwapChainImageIndex];

	////define size of render area -- should match size of attachments for best performance
	//renderPassInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
	//renderPassInfo.renderArea.extent = swapChainExtent;

	////size of clearValues and order, should match the order of attachments
	//std::array<vk::ClearValue, 2> clearValues{};
	//clearValues[0].color = std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f }; //clear color for background color will be used with VK_ATTACHMENT_LOAD_OP_CLEAR
	////depth values: 
	//	//0.0 - closest viewing plane 
	//	//1.0 - furthest possible depth
	//clearValues[1].depthStencil = vk::ClearDepthStencilValue{ 1.0, 0 };

	//renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	//renderPassInfo.pClearValues = clearValues.data();

	///* vkCmdBeginRenderPass */
	////Args: 
	//	//1. command buffer to set recording to 
	//	//2. details of the render pass
	//	//3. how drawing commands within the render pass will be provided
	//		//OPTIONS: 
	//			//VK_SUBPASS_CONTENTS_INLINE: render pass commands will be embedded in the primary command buffer. No secondary command buffers executed 
	//			//VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: render pass commands will be executed from the secondary command buffers
	//commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

	/* Drawing Commands */
	//Args: 
		//2. compute or graphics pipeline
		//3. pipeline object

	/* vkCmdBindDescriptorSets:
	*   1.
	*   2. Descriptor sets are not unique to graphics pipelines, must specify to use in graphics or compute pipelines.
	*   3. layout that the descriptors are based on
	*   4. index of first descriptor set
	*   5. number of sets to bind
	*   6. array of sets to bind
	*   7 - 8. array of offsets used for dynamic descriptors (not used here)
	*/
	//bind the right descriptor set for each swap chain image to the descripts in the shader
	//bind global descriptor

	vk::RenderingAttachmentInfoKHR colorAttachmentInfo{};
	colorAttachmentInfo.imageView = this->renderToImageViews[frameInFlightIndex];

	vk::RenderingAttachmentInfoKHR depthAttachmentInfo{}; 
	depthAttachmentInfo.imageView = this->depthImageView; 

	auto renderArea = vk::Rect2D{ vk::Offset2D{}, *this->swapChainExtent }; 
	vk::RenderingInfoKHR renderInfo{}; 
	renderInfo.layerCount = 1; 
	renderInfo.pStencilAttachment = &depthAttachmentInfo;
	renderInfo.pDepthAttachment = &depthAttachmentInfo; 
	renderInfo.pColorAttachments = &colorAttachmentInfo; 
	renderInfo.colorAttachmentCount = 1;

	commandBuffer.beginRendering(renderInfo); 

	for (auto& group : this->renderGroups) {
		group->recordRenderPassCommands(commandBuffer, frameInFlightIndex);
	}

	commandBuffer.endRenderPass();
}

Command_Buffer_Order SceneRenderer::getCommandBufferOrder()
{
	return Command_Buffer_Order::main_render_pass;
}

Command_Buffer_Type SceneRenderer::getCommandBufferType()
{
	return Command_Buffer_Type::Tgraphics;
}

vk::PipelineStageFlags SceneRenderer::getWaitStages()
{
	return vk::PipelineStageFlagBits::eVertexShader;
}

bool SceneRenderer::getWillBeSubmittedEachFrame()
{
	return true; 
}

bool SceneRenderer::getWillBeRecordedOnce()
{
	return false; 
}

void SceneRenderer::prepareForSubmission(const int& frameIndexToBeDrawn)
{
	updateUniformBuffer(frameIndexToBeDrawn);
}

std::optional<std::function<void(const int&)>> SceneRenderer::getBeforeBufferSubmissionCallback()
{
	return std::optional<std::function<void(const int&)>>(std::function<void(const int&)>(std::bind(&SceneRenderer::prepareForSubmission, this, std::placeholders::_1)));
}

}