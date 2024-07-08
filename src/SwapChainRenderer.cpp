#include "SwapChainRenderer.hpp"

typedef std::chrono::high_resolution_clock Clock;

namespace star {

SwapChainRenderer::SwapChainRenderer(StarWindow& window, std::vector<std::unique_ptr<Light>>& lightList, 
	std::vector<std::reference_wrapper<StarObject>> objectList, 
	StarCamera& camera, StarDevice& device) 
	: StarRenderer(window, camera, device), lightList(lightList), objectList(objectList){
	createSwapChain();
}

void SwapChainRenderer::prepare()
{
	queryDeviceSupport();
	createImageViews();
	createRenderPass();
	createRenderingBuffers();
	createDescriptors(); 
	createDepthResources();
	createFramebuffers();
	createRenderingGroups();
	createCommandBuffers();
	recordScreenshotCommandBuffers();
	createSemaphores();
	createFences();
	createFenceImageTracking();
}
	
void SwapChainRenderer::takeScreenshot(const uint32_t& currentBuffer)
{
	this->screenshotCommandBuffer->submit(currentBuffer);

	vk::ImageSubresource subResource{vk::ImageAspectFlagBits::eColor, 0, 0};
	vk::SubresourceLayout layout{}; 
	this->device.getDevice().getImageSubresourceLayout(this->copyDstImages[currentBuffer], &subResource, &layout);

	unsigned char* data = nullptr;
	vmaMapMemory(this->device.getAllocator(), this->copyDstImageMemories[currentBuffer], (void**)&data);
	Texture texture(this->swapChainExtent.width, this->swapChainExtent.height, layout, data);
	texture.saveToDisk(*this->screenshotPath);
	vmaUnmapMemory(this->device.getAllocator(), this->copyDstImageMemories[currentBuffer]);
}

void SwapChainRenderer::queryDeviceSupport()
{
	this->supportsBlit = this->deviceSupports_SwapchainBlit() && this->devieSupports_blitToLinearImage();
}

void SwapChainRenderer::createRenderingGroups()
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
			this->renderGroups.push_back(std::unique_ptr<StarRenderGroup>(new StarRenderGroup(device, MAX_FRAMES_IN_FLIGHT, 
				swapChainExtent, object)));
		}
	}

	//init all groups
	for (auto& group : this->renderGroups) {
		group->init(*globalSetLayout, this->renderPass, this->globalDescriptorSets);
	}
}

void SwapChainRenderer::updateUniformBuffer(uint32_t currentImage)
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

SwapChainRenderer::~SwapChainRenderer()
{
	cleanup(); 
}

void SwapChainRenderer::pollEvents()
{
	glfwPollEvents();
}

void SwapChainRenderer::submit()
{
	/* Goals of each call to drawFrame:
	   *   get an image from the swap chain
	   *   execute command buffer with that image as attachment in the framebuffer
	   *   return the image to swapchain for presentation
	   * by default each of these steps would be executed asynchronously so need method of synchronizing calls with rendering
	   * two ways of doing this:
	   *   1. fences
	   *       accessed through calls to vkWaitForFences
	   *       designed to synchronize application itself with rendering ops
	   *   2. semaphores
	   *       designed to synchronize opertaions within or across command queues
	   * need to sync queue operations of draw and presentation commmands -> using semaphores
	   */

	   //wait for fence to be ready 
	   // 3. 'VK_TRUE' -> waiting for all fences
	   // 4. timeout 

	this->device.getDevice().waitForFences(inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	/* Get Image From Swapchain */

	//as is extension we must use vk*KHR naming convention
	//UINT64_MAX -> 3rd argument: used to specify timeout in nanoseconds for image to become available
	/* Suboptimal SwapChain notes */
		//vulkan can return two different flags 
		// 1. VK_ERROR_OUT_OF_DATE_KHR: swap chain has become incompatible with the surface and cant be used for rendering. (Window resize)
		// 2. VK_SUBOPTIMAL_KHR: swap chain can still be used to present to the surface, but the surface properties no longer match
	auto result = this->device.getDevice().acquireNextImageKHR(swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame]);

	if (result.result == vk::Result::eErrorOutOfDateKHR) {
		//the swapchain is no longer optimal according to vulkan. Must recreate a more efficient swap chain
		recreateSwapChain();
		return;
	}
	else if (result.result != vk::Result::eSuccess && result.result != vk::Result::eSuboptimalKHR) {
		//for VK_SUBOPTIMAL_KHR can also recreate swap chain. However, chose to continue to presentation stage
		throw std::runtime_error("failed to acquire swap chain image");
	}
	uint32_t imageIndex = result.value;

	//check if a previous frame is using the current image
	if (imagesInFlight[imageIndex]) {
		this->device.getDevice().waitForFences(1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}
	//mark image as now being in use by this frame by assigning the fence to it 
	imagesInFlight[imageIndex] = inFlightFences[currentFrame];

	updateUniformBuffer(currentFrame);

	//set fence to unsignaled state
	this->device.getDevice().resetFences(1, &inFlightFences[currentFrame]);

	//re-record command buffer
	this->graphicsCommandBuffer->buffer(currentFrame).reset(); 
	recordCommandBuffer(currentFrame, imageIndex); 
	this->graphicsCommandBuffer->submit(currentFrame, inFlightFences[currentFrame], std::pair<vk::Semaphore, vk::PipelineStageFlags>(imageAvailableSemaphores[currentFrame], vk::PipelineStageFlagBits::eColorAttachmentOutput));

	const vk::Semaphore* doneSemaphore = nullptr;
	if (this->screenshotPath != nullptr) {
		doneSemaphore = &this->screenshotCommandBuffer->getCompleteSemaphores().at(currentFrame);

		takeScreenshot(currentFrame);
		this->screenshotPath = nullptr;
	}
	else {
		doneSemaphore = &this->graphicsCommandBuffer->getCompleteSemaphores().at(currentFrame);
	}

	/* Presentation */
	vk::PresentInfoKHR presentInfo{};
	presentInfo.sType = vk::StructureType::ePresentInfoKHR;

	//what to wait for 
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = doneSemaphore;

	//what swapchains to present images to 
	vk::SwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	//can use this to get results from swap chain to check if presentation was successful
	presentInfo.pResults = nullptr; // Optional

	//make call to present image
	auto presentResult = this->device.getPresentQueue().presentKHR(presentInfo);

	if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR || frameBufferResized) {
		frameBufferResized = false;
		recreateSwapChain();
	}
	else if (presentResult != vk::Result::eSuccess) {
		throw std::runtime_error("failed to present swap chain image");
	}

	//advance to next frame
	previousFrame = currentFrame; 
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}


void SwapChainRenderer::cleanup()
{
	cleanupSwapChain();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		this->device.getDevice().destroySemaphore(renderFinishedSemaphores[i]);
		this->device.getDevice().destroySemaphore(imageAvailableSemaphores[i]);
		this->device.getDevice().destroyFence(inFlightFences[i]);
	}
}

void SwapChainRenderer::cleanupSwapChain()
{
	this->device.getDevice().destroyImageView(this->depthImageView);
	vmaDestroyImage(this->device.getAllocator(), this->depthImage, this->depthImageMemory);

	for (int i = 0; i < this->copyDstImageMemories.size(); i++) {
		vmaDestroyImage(this->device.getAllocator(), this->copyDstImages[i], this->copyDstImageMemories[i]);
	}

	for (auto framebuffer : this->swapChainFramebuffers) {
		this->device.getDevice().destroyFramebuffer(framebuffer);
	}

	this->device.getDevice().destroyRenderPass(this->renderPass);

	for (auto imageView : this->swapChainImageViews) {
		this->device.getDevice().destroyImageView(imageView);
	}

	this->device.getDevice().destroySwapchainKHR(this->swapChain);
}

void SwapChainRenderer::recreateSwapChain()
{
	int width = 0, height = 0;

	//check for window minimization and wait for window size to no longer be 0
	glfwGetFramebufferSize(this->window.getGLFWwindow(), &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(this->window.getGLFWwindow(), &width, &height);
		glfwWaitEvents();
	}
	//wait for device to finish any current actions
	vkDeviceWaitIdle(this->device.getDevice());

	cleanupSwapChain();

	//create swap chain itself 
	createSwapChain();

	//image views depend directly on swap chain images so these need to be recreated
	createImageViews();

	//render pass depends on the format of swap chain images
	createRenderPass();

	createDepthResources();

	createFramebuffers();

	//uniform buffers are dependent on the number of swap chain images, will need to recreate since they are destroyed in cleanupSwapchain()
	createRenderingBuffers();

	createDescriptors();

	recordScreenshotCommandBuffers();
}

void SwapChainRenderer::createSwapChain()
{
	//TODO: current implementation requires halting to all rendering when recreating swapchain. Can place old swap chain in oldSwapChain field 
		//  in order to prevent this and allow rendering to continue
	SwapChainSupportDetails swapChainSupport = this->device.getSwapChainSupportDetails();

	vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	//how many images should be in the swap chain 
	//in order to avoid extra waiting for driver overhead, author of tutorial recommends +1 of the minimum
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

	//with this additional +1 make sure not to go over maximum permitted 
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	vk::SwapchainCreateInfoKHR createInfo{};
	createInfo.sType = vk::StructureType::eSwapchainCreateInfoKHR;
	//createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;     
	createInfo.surface = this->device.getSurface();

	//specify image information for the surface 
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1; //1 unless using 3D display 
	createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc; //how are these images going to be used? Color attachment since we are rendering to them (can change for postprocessing effects)

	QueueFamilyIndicies indicies = this->device.findPhysicalQueueFamilies();
	std::vector<uint32_t> queueFamilyIndicies;
	if (indicies.transferFamily.has_value())
		queueFamilyIndicies = std::vector<uint32_t>{ indicies.graphicsFamily.value(), indicies.transferFamily.value(), indicies.presentFamily.value() };
	else
		queueFamilyIndicies = std::vector<uint32_t>{ indicies.graphicsFamily.value(), indicies.presentFamily.value() };

	if (indicies.graphicsFamily != indicies.presentFamily && indicies.presentFamily != indicies.transferFamily) {
		/*need to handle how images will be transferred between different queues
		* so we need to draw images on the graphics queue and then submitting them to the presentation queue
		* Two ways of handling this:
		* 1. VK_SHARING_MODE_EXCLUSIVE: an image is owned by one queue family at a time and can be transferred between groups
		* 2. VK_SHARING_MODE_CONCURRENT: images can be used across queue families without explicit ownership
		*/
		//createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		createInfo.queueFamilyIndexCount = 3;
		createInfo.pQueueFamilyIndices = queueFamilyIndicies.data();
	}
	else if (indicies.graphicsFamily != indicies.presentFamily && indicies.presentFamily == indicies.transferFamily) {
		uint32_t explicitQueueFamilyInd[] = { indicies.graphicsFamily.value(), indicies.presentFamily.value() };
		createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndicies.data();
	}
	else {
		//same family is used for graphics and presenting
		createInfo.imageSharingMode = vk::SharingMode::eExclusive;
		createInfo.queueFamilyIndexCount = 0; //optional
		createInfo.pQueueFamilyIndices = nullptr; //optional
	}

	//can specify certain transforms if they are supported (like 90 degree clockwise rotation)
	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;

	//what present mode is going to be used
	createInfo.presentMode = presentMode;
	//if clipped is set to true, we dont care about color of pixes that arent in sight -- best performance to enable this
	createInfo.clipped = VK_TRUE;

	//for now, only assume we are making one swapchain
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	this->swapChain = this->device.getDevice().createSwapchainKHR(createInfo);

	//get images in the newly created swapchain 
	this->swapChainImages = this->device.getDevice().getSwapchainImagesKHR(this->swapChain);

	//save swapChain information for later use
	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

void SwapChainRenderer::createImageViews()
{
	swapChainImageViews.resize(swapChainImages.size());

	//need to create an imageView for each of the images available
	for (size_t i = 0; i < swapChainImages.size(); i++) {
		swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, vk::ImageAspectFlagBits::eColor);
	}
}

vk::ImageView SwapChainRenderer::createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlagBits aspectFlags)
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

void SwapChainRenderer::createDescriptors()
{
	this->globalSetLayout = StarDescriptorSetLayout::Builder(this->device)
		.addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eAll)
		.addBinding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eAll)
		.build();

	this->globalDescriptorSets.resize(this->swapChainImages.size());

	std::unique_ptr<std::vector<vk::DescriptorBufferInfo>> bufferInfos{};
	for (size_t i = 0; i < this->swapChainImages.size(); i++) {
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

void SwapChainRenderer::createRenderPass()
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
	colorAttachment.format = swapChainImageFormat;
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
	//renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
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

void SwapChainRenderer::createDepthResources()
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

void SwapChainRenderer::createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image, VmaAllocation& imageMemory)
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

void SwapChainRenderer::createFramebuffers()
{
	swapChainFramebuffers.resize(swapChainImageViews.size());

	//iterate through each image and create a buffer for it 
	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		std::array<vk::ImageView, 2> attachments = {
			swapChainImageViews[i],
			depthImageView //same depth image is going to be used for all swap chain images 
		};

		vk::FramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = vk::StructureType::eFramebufferCreateInfo;
		//make sure that framebuffer is compatible with renderPass (same # and type of attachments)
		framebufferInfo.renderPass = renderPass;
		//specify which vkImageView objects to bind to the attachment descriptions in the render pass pAttachment array
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1; //# of layers in image arrays

		swapChainFramebuffers[i] = this->device.getDevice().createFramebuffer(framebufferInfo);
		if (!swapChainFramebuffers[i]) {
			throw std::runtime_error("failed to create framebuffer");
		}
	}
}

void SwapChainRenderer::createRenderingBuffers()
{
	vk::DeviceSize globalBufferSize = sizeof(GlobalUniformBufferObject) * this->objectList.size();

	this->globalUniformBuffers.resize(this->swapChainImages.size());
	if (this->lightList.size() > 0) {
		this->lightBuffers.resize(this->swapChainImages.size());
	}

	for (size_t i = 0; i < swapChainImages.size(); i++) {
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

void SwapChainRenderer::createCommandBuffers()
{
	this->graphicsCommandBuffer = std::make_unique<StarCommandBuffer>(device, MAX_FRAMES_IN_FLIGHT, Command_Buffer_Type::Tgraphics);
	if (this->supportsBlit)
		this->screenshotCommandBuffer = std::make_unique<StarCommandBuffer>(device, MAX_FRAMES_IN_FLIGHT, Command_Buffer_Type::Tgraphics);
	else
		this->screenshotCommandBuffer = std::make_unique<StarCommandBuffer>(device, MAX_FRAMES_IN_FLIGHT, Command_Buffer_Type::Ttransfer);
	this->screenshotCommandBuffer->waitFor(*this->graphicsCommandBuffer, vk::PipelineStageFlagBits::eTransfer); 

	this->copyDstImageMemories.resize(MAX_FRAMES_IN_FLIGHT);
	this->copyDstImages.resize(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
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

void SwapChainRenderer::recordCommandBuffer(uint32_t bufferIndex, uint32_t imageIndex){
		this->graphicsCommandBuffer->begin(bufferIndex); 

		vk::Viewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)this->swapChainExtent.width;
		viewport.height = (float)this->swapChainExtent.height;
		//Specify values range of depth values to use for the framebuffer. If not doing anything special, leave at default
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		this->graphicsCommandBuffer->buffer(bufferIndex).setViewport(0, viewport);

		for (auto& group : this->renderGroups) {
			group->recordPreRenderPassCommands(*this->graphicsCommandBuffer, bufferIndex);
		}

		/* Begin render pass */
		//drawing starts by beginning a render pass 
		vk::RenderPassBeginInfo renderPassInfo{};
		//renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.sType = vk::StructureType::eRenderPassBeginInfo;

		//define the render pass we want
		renderPassInfo.renderPass = renderPass;

		//what attachments do we need to bind
		//previously created swapChainbuffers to hold this information 
		renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];

		//define size of render area -- should match size of attachments for best performance
		renderPassInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
		renderPassInfo.renderArea.extent = swapChainExtent;

		//size of clearValues and order, should match the order of attachments
		std::array<vk::ClearValue, 2> clearValues{};
		clearValues[0].color = std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f }; //clear color for background color will be used with VK_ATTACHMENT_LOAD_OP_CLEAR
		//depth values: 
			//0.0 - closest viewing plane 
			//1.0 - furthest possible depth
		clearValues[1].depthStencil = vk::ClearDepthStencilValue{ 1.0, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		/* vkCmdBeginRenderPass */
		//Args: 
			//1. command buffer to set recording to 
			//2. details of the render pass
			//3. how drawing commands within the render pass will be provided
				//OPTIONS: 
					//VK_SUBPASS_CONTENTS_INLINE: render pass commands will be embedded in the primary command buffer. No secondary command buffers executed 
					//VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: render pass commands will be executed from the secondary command buffers
		this->graphicsCommandBuffer->buffer(bufferIndex).beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

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
		vk::DeviceSize offsets{};
		//this->graphicsCommandBuffer->buffer(bufferIndex).bindVertexBuffers(0, this->vertexBuffer->getBuffer(), offsets);
		//this->graphicsCommandBuffer->buffer(bufferIndex).bindIndexBuffer(this->indexBuffer->getBuffer(), 0, vk::IndexType::eUint32);

		for (auto& group : this->renderGroups) {
			group->recordRenderPassCommands(*this->graphicsCommandBuffer, bufferIndex);
		}

		this->graphicsCommandBuffer->buffer(bufferIndex).endRenderPass();

		//record command buffer
		this->graphicsCommandBuffer->buffer(bufferIndex).end();
}

void SwapChainRenderer::createSemaphores()
{
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

	vk::SemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = vk::StructureType::eSemaphoreCreateInfo;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		this->imageAvailableSemaphores[i] = this->device.getDevice().createSemaphore(semaphoreInfo);
		this->renderFinishedSemaphores[i] = this->device.getDevice().createSemaphore(semaphoreInfo);

		if (!this->imageAvailableSemaphores[i]) {
			throw std::runtime_error("failed to create semaphores for a frame");
		}
	}
}

void SwapChainRenderer::createFences()
{
	//note: fence creation can be rolled into semaphore creation. Seperated for understanding
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	vk::FenceCreateInfo fenceInfo{};
	fenceInfo.sType = vk::StructureType::eFenceCreateInfo;

	//create the fence in a signaled state 
	//fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		this->inFlightFences[i] = this->device.getDevice().createFence(fenceInfo);
		if (!this->inFlightFences[i]) {
			throw std::runtime_error("failed to create fence object for a frame");
		}
	}
}

void SwapChainRenderer::recordScreenshotCommandBuffers()
{
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vk::Image srcImage = this->swapChainImages[i];

		this->screenshotCommandBuffer->begin(i);
		auto commandBuffer = this->screenshotCommandBuffer->buffer(i);

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

			barrier.image = this->copyDstImages[i];
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

			commandBuffer.blitImage(srcImage, vk::ImageLayout::eTransferSrcOptimal, this->copyDstImages[i],
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

			commandBuffer.copyImage(srcImage, vk::ImageLayout::eTransferSrcOptimal, this->copyDstImages[i], vk::ImageLayout::eTransferDstOptimal, 1, &copyRegion);
		}

		//need to transition images back to general layout for next frame use
		{
			//destination image
			vk::ImageMemoryBarrier barrier{};
			barrier.sType = vk::StructureType::eImageMemoryBarrier;
			barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
			barrier.newLayout = vk::ImageLayout::eGeneral;
			barrier.image = this->copyDstImages[i];
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
}

void SwapChainRenderer::createFenceImageTracking()
{
	//note: just like createFences() this too can be wrapped into semaphore creation. Seperated for understanding.

	//need to ensure the frame that is going to be drawn to, is the one linked to the expected fence.
	//If, for any reason, vulkan returns an image out of order, we will be able to handle that with this link
	imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

	//initially, no frame is using any image so this is going to be created without an explicit link
}


vk::SurfaceFormatKHR SwapChainRenderer::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
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

vk::PresentModeKHR SwapChainRenderer::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
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
vk::Extent2D SwapChainRenderer::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
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
vk::Format SwapChainRenderer::findDepthFormat()
{
	//utilizing the VK_FORMAT_FEATURE_ flag to check for candidates that have a depth component.
	return this->device.findSupportedFormat(
		{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
		vk::ImageTiling::eOptimal,
		vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}
void SwapChainRenderer::initResources(StarDevice& device, const int& numFramesInFlight)
{
	ManagerDescriptorPool::request(vk::DescriptorType::eUniformBuffer, this->swapChainImages.size());
	ManagerDescriptorPool::request(vk::DescriptorType::eStorageBuffer, this->swapChainImages.size());
}
void SwapChainRenderer::destroyResources(StarDevice& device)
{
}
bool SwapChainRenderer::deviceSupports_SwapchainBlit()
{
	bool supportsBlit = true;

	vk::FormatProperties formatProperties = this->device.getPhysicalDevice().getFormatProperties(swapChainImageFormat);
	if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitSrc)) {
		supportsBlit = false;
	}

	return supportsBlit;
}
bool SwapChainRenderer::devieSupports_blitToLinearImage()
{
	bool supportsBlit = true; 

	vk::FormatProperties formatProperties = this->device.getPhysicalDevice().getFormatProperties(vk::Format::eR8G8B8A8Snorm);
	if (!(formatProperties.linearTilingFeatures & vk::FormatFeatureFlagBits::eBlitDst)) {
		supportsBlit = false;
	}

	return supportsBlit;
}
}