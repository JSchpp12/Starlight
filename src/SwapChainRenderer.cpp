#include "SwapChainRenderer.hpp"
#include "SwapChainRenderer.hpp"
#include "SwapChainRenderer.hpp"
#include "SwapChainRenderer.hpp"

star::SwapChainRenderer::SwapChainRenderer(StarWindow& window, std::vector<std::unique_ptr<Light>>& lightList, std::vector<std::reference_wrapper<StarObject>> objectList, StarCamera& camera, StarDevice& device)
	: window(window), SceneRenderer(lightList, objectList, camera, device)
{
	createSwapChain(); 
}

star::SwapChainRenderer::~SwapChainRenderer()
{
	cleanupSwapChain();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		this->device.getDevice().destroySemaphore(imageAvailableSemaphores[i]);
		this->device.getDevice().destroyFence(inFlightFences[i]);
	}
}

void star::SwapChainRenderer::prepare()
{
	auto numSwapChainImages = this->device.getDevice().getSwapchainImagesKHR(this->swapChain).size(); 
	this->SceneRenderer::prepare(this->swapChainExtent, numSwapChainImages, this->swapChainImageFormat);

	this->createSemaphores(); 
	this->createFences();
	this->createFenceImageTracking(); 
}

void star::SwapChainRenderer::submitPresentation(const int& frameIndexToBeDrawn, const vk::Semaphore* mainGraphicsDoneSemaphore)
{
	/* Presentation */
	vk::PresentInfoKHR presentInfo{};
	presentInfo.sType = vk::StructureType::ePresentInfoKHR;

	//what to wait for 
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = mainGraphicsDoneSemaphore;

	//what swapchains to present images to 
	vk::SwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &this->currentSwapChainImageIndex;

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
}

void star::SwapChainRenderer::pollEvents() {
	glfwPollEvents(); 
}

vk::SurfaceFormatKHR star::SwapChainRenderer::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
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

vk::PresentModeKHR star::SwapChainRenderer::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
{
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

vk::Extent2D star::SwapChainRenderer::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
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

void star::SwapChainRenderer::prepareForSubmission(const int& frameIndexToBeDrawn)
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

	this->device.getDevice().waitForFences(inFlightFences[frameIndexToBeDrawn], VK_TRUE, UINT64_MAX);

	/* Get Image From Swapchain */

	//as is extension we must use vk*KHR naming convention
	//UINT64_MAX -> 3rd argument: used to specify timeout in nanoseconds for image to become available
	/* Suboptimal SwapChain notes */
		//vulkan can return two different flags 
		// 1. VK_ERROR_OUT_OF_DATE_KHR: swap chain has become incompatible with the surface and cant be used for rendering. (Window resize)
		// 2. VK_SUBOPTIMAL_KHR: swap chain can still be used to present to the surface, but the surface properties no longer match
	auto result = this->device.getDevice().acquireNextImageKHR(swapChain, UINT64_MAX, imageAvailableSemaphores[frameIndexToBeDrawn]);

	if (result.result == vk::Result::eErrorOutOfDateKHR) {
		//the swapchain is no longer optimal according to vulkan. Must recreate a more efficient swap chain
		recreateSwapChain();
		return;
	}
	else if (result.result != vk::Result::eSuccess && result.result != vk::Result::eSuboptimalKHR) {
		//for VK_SUBOPTIMAL_KHR can also recreate swap chain. However, chose to continue to presentation stage
		throw std::runtime_error("failed to acquire swap chain image");
	}
	this->currentSwapChainImageIndex = result.value;

	//check if a previous frame is using the current image
	if (imagesInFlight[this->currentSwapChainImageIndex]) {
		this->device.getDevice().waitForFences(1, &imagesInFlight[this->currentSwapChainImageIndex], VK_TRUE, UINT64_MAX);
	}
	//mark image as now being in use by this frame by assigning the fence to it 
	imagesInFlight[this->currentSwapChainImageIndex] = inFlightFences[frameIndexToBeDrawn];

	this->SceneRenderer::prepareForSubmission(frameIndexToBeDrawn); 

	//set fence to unsignaled state
	this->device.getDevice().resetFences(1, &inFlightFences[frameIndexToBeDrawn]);
}

void star::SwapChainRenderer::submissionDone()
{
	//advance to next frame
	previousFrame = currentFrame;
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void star::SwapChainRenderer::submitBuffer(StarCommandBuffer& buffer, const int& frameIndexToBeDrawn)
{
	buffer.submit(frameIndexToBeDrawn, inFlightFences[frameIndexToBeDrawn], std::pair<vk::Semaphore, vk::PipelineStageFlags>(imageAvailableSemaphores[currentFrame], vk::PipelineStageFlagBits::eColorAttachmentOutput));
}

std::optional<std::function<void(const int&)>> star::SwapChainRenderer::getAfterBufferSubmissionCallback()
{
	return std::optional<std::function<void(const int&)>>(std::bind(&SwapChainRenderer::submissionDone, this));
}

std::optional<std::function<void(star::StarCommandBuffer&, const int&)>> star::SwapChainRenderer::getOverrideBufferSubmissionCallback()
{
	return std::optional<std::function<void(StarCommandBuffer&, const int&)>>(std::bind(&SwapChainRenderer::submitBuffer, this, std::placeholders::_1, std::placeholders::_2));
}

std::vector<vk::Image> star::SwapChainRenderer::createRenderToImages()
{
	//get images in the newly created swapchain 
	return this->device.getDevice().getSwapchainImagesKHR(this->swapChain);
}

vk::RenderingAttachmentInfo star::SwapChainRenderer::prepareDynamicRenderingInfoColorAttachment(const int& frameInFlightIndex)
{
	vk::RenderingAttachmentInfoKHR colorAttachmentInfo{};
	colorAttachmentInfo.imageView = this->renderToImageViews[this->currentSwapChainImageIndex];
	colorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	colorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachmentInfo.clearValue = vk::ClearValue{ vk::ClearValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}} };

	return colorAttachmentInfo;
}

void star::SwapChainRenderer::recordCommandBuffer(vk::CommandBuffer& commandBuffer, const int& frameInFlightIndex)
{
	//transition image layout

	//for presentation will need to change layout of the image to presentation
 	vk::ImageMemoryBarrier presentBarrier{};
	presentBarrier.sType = vk::StructureType::eImageMemoryBarrier;
	presentBarrier.oldLayout = vk::ImageLayout::eUndefined;
	presentBarrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
	presentBarrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
	presentBarrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
	presentBarrier.image = this->renderToImages[this->currentSwapChainImageIndex];
	presentBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	presentBarrier.subresourceRange.baseMipLevel = 0;
	presentBarrier.subresourceRange.levelCount = 1;
	presentBarrier.subresourceRange.baseArrayLayer = 0;
	presentBarrier.subresourceRange.layerCount = 1;

	commandBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eFragmentShader,
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
		{}, {}, nullptr, presentBarrier
	);

	this->SceneRenderer::recordCommandBuffer(commandBuffer, frameInFlightIndex);
}

void star::SwapChainRenderer::createSemaphores()
{
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

	vk::SemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = vk::StructureType::eSemaphoreCreateInfo;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		this->imageAvailableSemaphores[i] = this->device.getDevice().createSemaphore(semaphoreInfo);

		if (!this->imageAvailableSemaphores[i]) {
			throw std::runtime_error("failed to create semaphores for a frame");
		}
	}
}

void star::SwapChainRenderer::createFences()
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

void star::SwapChainRenderer::createFenceImageTracking()
{
	//note: just like createFences() this too can be wrapped into semaphore creation. Seperated for understanding.

	//need to ensure the frame that is going to be drawn to, is the one linked to the expected fence.
	//If, for any reason, vulkan returns an image out of order, we will be able to handle that with this link
	imagesInFlight.resize(this->renderToImages.size(), VK_NULL_HANDLE);

	//initially, no frame is using any image so this is going to be created without an explicit link
}

void star::SwapChainRenderer::createSwapChain()
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

	//save swapChain information for later use
	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

void star::SwapChainRenderer::recreateSwapChain()
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
	//createImageViews();

	//render pass depends on the format of swap chain images
	//createRenderPass();

	//createFramebuffers();

	////uniform buffers are dependent on the number of swap chain images, will need to recreate since they are destroyed in cleanupSwapchain()
	//createRenderingBuffers();

	//createDescriptors();
}

void star::SwapChainRenderer::cleanupSwapChain()
{
	this->device.getDevice().destroyImageView(this->depthImageView);
	vmaDestroyImage(this->device.getAllocator(), this->depthImage, this->depthImageMemory);

	//for (auto framebuffer : this->swapChainFramebuffers) {
	//	this->device.getDevice().destroyFramebuffer(framebuffer);
	//}

	//this->device.getDevice().destroyRenderPass(this->renderPass);

	//for (auto imageView : this->swapChainImageViews) {
	//	this->device.getDevice().destroyImageView(imageView);
	//}

	this->device.getDevice().destroySwapchainKHR(this->swapChain);
}
