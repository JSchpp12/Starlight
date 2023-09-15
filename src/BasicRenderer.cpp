#include "BasicRenderer.hpp"

namespace star {

BasicRenderer::BasicRenderer(StarWindow& window, std::vector<Light*> inLightList) :
	StarRenderer(window), lightList(inLightList)
{
	device = StarDevice::New(window); 
}

void BasicRenderer::prepare()
{
	createSwapChain(); 
	createImageViews();
	createRenderPass();

	this->globalPool = StarDescriptorPool::Builder(*this->device.get())
		.setMaxSets(15)
		.addPoolSize(vk::DescriptorType::eUniformBuffer, this->swapChainImages.size() * this->swapChainImages.size())
		.addPoolSize(vk::DescriptorType::eStorageBuffer, this->lightList.size() * this->swapChainImages.size())
		.build();
}

BasicRenderer::~BasicRenderer()
{
	device->getDevice().waitIdle(); 
	cleanup(); 
}

void BasicRenderer::cleanup()
{
	cleanupSwapChain(); 
}

void BasicRenderer::cleanupSwapChain()
{
	//this->device->getDevice().destroyImageView(this->depthImageView);
	//this->device->getDevice().destroyImage(this->depthImage);
	//this->device->getDevice().freeMemory(this->depthImageMemory);

	for (auto framebuffer : this->swapChainFramebuffers) {
		this->device->getDevice().destroyFramebuffer(framebuffer);
	}

	this->device->getDevice().destroyRenderPass(this->renderPass);

	for (auto imageView : this->swapChainImageViews) {
		this->device->getDevice().destroyImageView(imageView);
	}

	this->device->getDevice().destroySwapchainKHR(this->swapChain);
}

void BasicRenderer::createSwapChain()
{
	//TODO: current implementation requires halting to all rendering when recreating swapchain. Can place old swap chain in oldSwapChain field 
		//  in order to prevent this and allow rendering to continue
	SwapChainSupportDetails swapChainSupport = this->device->getSwapChainSupportDetails();

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
	createInfo.surface = this->device->getSurface();

	//specify image information for the surface 
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1; //1 unless using 3D display 
	//createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; //how are these images going to be used? Color attachment since we are rendering to them (can change for postprocessing effects)
	createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment; //how are these images going to be used? Color attachment since we are rendering to them (can change for postprocessing effects)

	QueueFamilyIndicies indicies = this->device->findPhysicalQueueFamilies();
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

	this->swapChain = this->device->getDevice().createSwapchainKHR(createInfo);

	//if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
	//    throw std::runtime_error("failed to create swap chain");
	//}

	//get images in the newly created swapchain 
	this->swapChainImages = this->device->getDevice().getSwapchainImagesKHR(this->swapChain);

	//save swapChain information for later use
	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

void BasicRenderer::createImageViews()
{
	swapChainImageViews.resize(swapChainImages.size());

	//need to create an imageView for each of the images available
	for (size_t i = 0; i < swapChainImages.size(); i++) {
		//swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, vk::ImageAspectFlagBits::eColor);
	}
}

vk::ImageView BasicRenderer::createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlagBits aspectFlags)
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

	vk::ImageView imageView = this->device->getDevice().createImageView(viewInfo);

	if (!imageView) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

void BasicRenderer::createRenderPass()
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

	this->renderPass = this->device->getDevice().createRenderPass(renderPassInfo);
	if (!renderPass) {
		throw std::runtime_error("failed to create render pass");
	}
}



}