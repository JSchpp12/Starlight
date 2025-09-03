#include "renderer/SwapChainRenderer.hpp"

#include "ConfigFile.hpp"

#include <GLFW/glfw3.h>

star::core::renderer::SwapChainRenderer::SwapChainRenderer(core::device::DeviceContext &context,
                                                           const uint8_t &numFramesInFlight,
                                                           std::vector<std::shared_ptr<StarObject>> objects,
                                                           std::vector<std::shared_ptr<Light>> lights,
                                                           std::vector<Handle> &cameraInfoBuffers,
                                                           const StarWindow &window)
    : Renderer(context, numFramesInFlight, objects, lights, cameraInfoBuffers), window(window),
      numFramesInFlight(numFramesInFlight), device(context)
{
    createSwapChain();
}

star::core::renderer::SwapChainRenderer::SwapChainRenderer(core::device::DeviceContext &context,
                                                           const uint8_t &numFramesInFlight,
                                                           std::vector<std::shared_ptr<StarObject>> objects,
                                                           std::vector<std::shared_ptr<Light>> lights,
                                                           std::shared_ptr<StarCamera> camera, const StarWindow &window)
    : Renderer(context, numFramesInFlight, objects, lights, camera), window(window), numFramesInFlight(numFramesInFlight), device(context)
{
    createSwapChain();
}

star::core::renderer::SwapChainRenderer::~SwapChainRenderer()
{
    cleanupSwapChain();

    for (auto &semaphore : this->imageAvailableSemaphores)
    {
        this->device.getDevice().getVulkanDevice().destroySemaphore(semaphore);
    }

    for (auto &semaphore : this->imageAcquireSemaphores)
    {
        this->device.getDevice().getVulkanDevice().destroySemaphore(semaphore);
    }

    for (size_t i = 0; i < this->numFramesInFlight; i++)
    {
        this->device.getDevice().getVulkanDevice().destroyFence(inFlightFences[i]);
    }
}

void star::core::renderer::SwapChainRenderer::prepare(core::device::DeviceContext &device,
                                                      const vk::Extent2D &swapChainExtent, const int &numFramesInFlight)
{
    const size_t numSwapChainImages =
        this->device.getDevice().getVulkanDevice().getSwapchainImagesKHR(this->swapChain).size();
    this->Renderer::prepare(this->device, *this->swapChainExtent, numFramesInFlight);

    this->imageAcquireSemaphores = CreateSemaphores(this->device, numFramesInFlight);
    this->imageAvailableSemaphores = CreateSemaphores(this->device, numSwapChainImages);

    this->createFences();
    this->createFenceImageTracking();
}

void star::core::renderer::SwapChainRenderer::submitPresentation(const int &frameIndexToBeDrawn,
                                                                 const vk::Semaphore *mainGraphicsDoneSemaphore)
{
    /* Presentation */
    vk::PresentInfoKHR presentInfo{};
    presentInfo.sType = vk::StructureType::ePresentInfoKHR;

    // what to wait for
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = mainGraphicsDoneSemaphore;

    // what swapchains to present images to
    vk::SwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &this->currentSwapChainImageIndex;

    // can use this to get results from swap chain to check if presentation was successful
    presentInfo.pResults = nullptr; // Optional

    // make call to present image
    auto presentResult =
        this->device.getDevice().getDefaultQueue(star::Queue_Type::Tpresent).getVulkanQueue().presentKHR(presentInfo);

    if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR ||
        frameBufferResized)
    {
        frameBufferResized = false;
        recreateSwapChain();
    }
    else if (presentResult != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to present swap chain image");
    }

    // advance to next frame
    previousFrame = this->currentFrameInFlightCounter;
    this->currentFrameInFlightCounter = (this->currentFrameInFlightCounter + 1) % this->numFramesInFlight;
}

void star::core::renderer::SwapChainRenderer::pollEvents()
{
    glfwPollEvents();
}

star::core::device::managers::ManagerCommandBuffer::Request star::core::renderer::SwapChainRenderer::
    getCommandBufferRequest()
{
    return core::device::managers::ManagerCommandBuffer::Request{
        .recordBufferCallback =
            std::bind(&SwapChainRenderer::recordCommandBuffer, this, std::placeholders::_1, std::placeholders::_2),
        .order = Command_Buffer_Order::main_render_pass,
        .orderIndex = 0,
        .type = Queue_Type::Tgraphics,
        .waitStage = vk::PipelineStageFlagBits::eFragmentShader,
        .willBeSubmittedEachFrame = true,
        .recordOnce = false,
        .beforeBufferSubmissionCallback =
            std::bind(&SwapChainRenderer::prepareForSubmission, this, std::placeholders::_1),
        .overrideBufferSubmissionCallback = std::bind(&SwapChainRenderer::submitBuffer, this, std::placeholders::_1,
                                                      std::placeholders::_2, std::placeholders::_3)};
}

vk::SurfaceFormatKHR star::core::renderer::SwapChainRenderer::chooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR> &availableFormats) const
{
    for (const auto &availableFormat : availableFormats)
    {
        // check if a format allows 8 bits for R,G,B, and alpha channel
        // use SRGB color space

        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            return availableFormat;
        }
    }

    // if nothing matches what we are looking for, just take what is available
    return availableFormats[0];
}

vk::Format star::core::renderer::SwapChainRenderer::getColorAttachmentFormat(
    star::core::device::DeviceContext &device) const
{
    core::SwapChainSupportDetails swapChainSupport = device.getSwapchainSupportDetails();

    vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    return surfaceFormat.format;
}

vk::PresentModeKHR star::core::renderer::SwapChainRenderer::chooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR> &availablePresentModes)
{
    /*
     * There are a number of swap modes that are in vulkan
     * 1. VK_PRESENT_MODE_IMMEDIATE_KHR: images submitted by application are sent to the screen right away -- can cause
     * tearing
     * 2. VK_PRESENT_MODE_FIFO_RELAXED_KHR: images are placed in a queue and images are sent to the display in time with
     * display refresh (VSYNC like). If queue is full, application has to wait "Vertical blank" -> time when the display
     * is refreshed
     * 3. VK_PRESENT_MODE_FIFO_RELAXED_KHR: same as above. Except if the application is late, and the queue is empty:
     * the next image submitted is sent to display right away instead of waiting for next blank.
     * 4. VK_PRESENT_MODE_MAILBOX_KHR: similar to #2 option. Instead of blocking applicaiton when the queue is full, the
     * images in the queue are replaced with newer images. This mode can be used to render frames as fast as possible
     * while still avoiding tearing. Kind of like "tripple buffering". Does not mean that framerate is unlocked however.
     *   Author of tutorial statement: this mode [4] is a good tradeoff if energy use is not a concern. On mobile
     * devices it might be better to go with [2]
     */

    for (const auto &availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox)
        {
            return availablePresentMode;
        }
    }

    // only VK_PRESENT_MODE_FIFO_KHR is guaranteed to be available
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D star::core::renderer::SwapChainRenderer::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities)
{
    /*
     * "swap extent" -> resolution of the swap chain images (usually the same as window resultion
     * Range of available resolutions are defined in VkSurfaceCapabilitiesKHR
     * Resolution can be changed by setting value in currentExtent to the maximum value of a uint32_t
     *   then: the resolution can be picked by matching window size in minImageExtent and maxImageExtent
     */
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }
    else
    {
        // vulkan requires that resultion be defined in pixels -- if a high DPI display is used, screen coordinates do
        // not match with pixels
        int width, height;
        glfwGetFramebufferSize(this->window.getGLFWwindow(), &width, &height);

        vk::Extent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

        //(clamp) -- keep the width and height bounded by the permitted resolutions
        actualExtent.width =
            std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height =
            std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void star::core::renderer::SwapChainRenderer::prepareForSubmission(const int &frameIndexToBeDrawn)
{
    /* Goals of each call to drawFrame:
     *   get an image from the swap chain
     *   execute command buffer with that image as attachment in the framebuffer
     *   return the image to swapchain for presentation
     * by default each of these steps would be executed asynchronously so need method of synchronizing calls with
     * rendering two ways of doing this:
     *   1. fences
     *       accessed through calls to vkWaitForFences
     *       designed to synchronize application itself with rendering ops
     *   2. semaphores
     *       designed to synchronize opertaions within or across command queues
     * need to sync queue operations of draw and presentation commmands -> using semaphores
     */

    // wait for fence to be ready
    //  3. 'VK_TRUE' -> waiting for all fences
    //  4. timeout

    {
        auto result = this->device.getDevice().getVulkanDevice().waitForFences(inFlightFences[frameIndexToBeDrawn],
                                                                               VK_TRUE, UINT64_MAX);
        if (result != vk::Result::eSuccess)
            throw std::runtime_error("Failed to wait for fences");
    }

    /* Get Image From Swapchain */

    // as is extension we must use vk*KHR naming convention
    // UINT64_MAX -> 3rd argument: used to specify timeout in nanoseconds for image to become available
    /* Suboptimal SwapChain notes */
    // vulkan can return two different flags
    //  1. VK_ERROR_OUT_OF_DATE_KHR: swap chain has become incompatible with the surface and cant be used for rendering.
    //  (Window resize)
    //  2. VK_SUBOPTIMAL_KHR: swap chain can still be used to present to the surface, but the surface properties no
    //  longer match
    {
        auto result = this->device.getDevice().getVulkanDevice().acquireNextImageKHR(
            swapChain, UINT64_MAX, this->imageAcquireSemaphores[frameIndexToBeDrawn]);

        if (result.result == vk::Result::eErrorOutOfDateKHR)
        {
            // the swapchain is no longer optimal according to vulkan. Must recreate a more efficient swap chain
            recreateSwapChain();
            return;
        }
        else if (result.result != vk::Result::eSuccess && result.result != vk::Result::eSuboptimalKHR)
        {
            // for VK_SUBOPTIMAL_KHR can also recreate swap chain. However, chose to continue to presentation stage
            throw std::runtime_error("failed to acquire swap chain image");
        }

        this->currentSwapChainImageIndex = result.value;
    }

    // check if a previous frame is using the current image
    if (imagesInFlight[this->currentSwapChainImageIndex])
    {
        const vk::Result result = this->device.getDevice().getVulkanDevice().waitForFences(
            1, &imagesInFlight[this->currentSwapChainImageIndex], VK_TRUE, UINT64_MAX);
        if (result != vk::Result::eSuccess)
            throw std::runtime_error("Failed to wait for fences");
    }
    // mark image as now being in use by this frame by assigning the fence to it
    imagesInFlight[this->currentSwapChainImageIndex] = inFlightFences[frameIndexToBeDrawn];

    this->Renderer::prepareForSubmission(frameIndexToBeDrawn);

    // set fence to unsignaled state
    const vk::Result resetResult =
        this->device.getDevice().getVulkanDevice().resetFences(1, &inFlightFences[frameIndexToBeDrawn]);
    if (resetResult != vk::Result::eSuccess)
        throw std::runtime_error("Failed to reset fences");
}

vk::Semaphore star::core::renderer::SwapChainRenderer::submitBuffer(StarCommandBuffer &buffer,
                                                                    const int &frameIndexToBeDrawn,
                                                                    std::vector<vk::Semaphore> mustWaitFor)
{
    vk::SubmitInfo submitInfo{};
    std::vector<vk::Semaphore> waitSemaphores = {this->imageAcquireSemaphores[frameIndexToBeDrawn]};
    std::vector<vk::PipelineStageFlags> waitStages = {vk::PipelineStageFlagBits::eColorAttachmentOutput};

    for (auto &semaphore : mustWaitFor)
    {
        waitSemaphores.push_back(semaphore);
        waitStages.push_back(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    }

    submitInfo.commandBufferCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &this->imageAvailableSemaphores[this->currentSwapChainImageIndex];
    submitInfo.pWaitDstStageMask = waitStages.data();
    submitInfo.pCommandBuffers = &buffer.buffer(frameIndexToBeDrawn);
    submitInfo.commandBufferCount = 1;

    auto commandResult = std::make_unique<vk::Result>(this->device.getDevice()
                                                          .getDefaultQueue(star::Queue_Type::Tpresent)
                                                          .getVulkanQueue()
                                                          .submit(1, &submitInfo, inFlightFences[frameIndexToBeDrawn]));

    if (*commandResult != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to submit command buffer");
    }

    return this->imageAvailableSemaphores[this->currentSwapChainImageIndex];
}

std::vector<std::unique_ptr<star::StarTextures::Texture>> star::core::renderer::SwapChainRenderer::createRenderToImages(
    star::core::device::DeviceContext &device, const int &numFramesInFlight)
{
    std::vector<std::unique_ptr<StarTextures::Texture>> newRenderToImages =
        std::vector<std::unique_ptr<StarTextures::Texture>>();

    vk::Format format = getColorAttachmentFormat(device);

    // get images in the newly created swapchain
    for (vk::Image &image : this->device.getDevice().getVulkanDevice().getSwapchainImagesKHR(this->swapChain))
    {
        auto builder = star::StarTextures::Texture::Builder(device.getDevice().getVulkanDevice(), image)
                           .setBaseFormat(format)
                           .addViewInfo(vk::ImageViewCreateInfo()
                                            .setViewType(vk::ImageViewType::e2D)
                                            .setFormat(format)
                                            .setSubresourceRange(vk::ImageSubresourceRange()
                                                                     .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                                     .setBaseArrayLayer(0)
                                                                     .setLayerCount(1)
                                                                     .setBaseMipLevel(0)
                                                                     .setLevelCount(1)));

        newRenderToImages.emplace_back(builder.build());

        // auto buffer = device.beginSingleTimeCommands();
        // newRenderToImages.back()->transitionLayout(buffer, vk::ImageLayout::ePresentSrcKHR,
        // vk::AccessFlagBits::eNone, vk::AccessFlagBits::eColorAttachmentWrite,
        // vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput);
        // device.endSingleTimeCommands(buffer);
        auto oneTimeSetup = device.getDevice().beginSingleTimeCommands();

        vk::ImageMemoryBarrier barrier{};
        barrier.sType = vk::StructureType::eImageMemoryBarrier;
        barrier.oldLayout = vk::ImageLayout::eUndefined;
        barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

        barrier.image = newRenderToImages.back()->getVulkanImage();
        barrier.srcAccessMask = vk::AccessFlagBits::eNone;
        barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead;

        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseMipLevel = 0; // image does not have any mipmap levels
        barrier.subresourceRange.levelCount = 1;   // image is not an array
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        oneTimeSetup->buffer().pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,             // which pipeline stages should
                                                               // occurr before barrier
            vk::PipelineStageFlagBits::eColorAttachmentOutput, // pipeline stage in
                                                               // which operations will
                                                               // wait on the barrier
            {}, {}, nullptr, barrier);

        device.getDevice().endSingleTimeCommands(std::move(oneTimeSetup));
    }

    return newRenderToImages;
}

vk::RenderingAttachmentInfo star::core::renderer::SwapChainRenderer::prepareDynamicRenderingInfoColorAttachment(
    const int &frameInFlightIndex)
{
    vk::RenderingAttachmentInfoKHR colorAttachmentInfo{};
    colorAttachmentInfo.imageView = this->renderToImages[this->currentSwapChainImageIndex]->getImageView();
    colorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachmentInfo.clearValue = vk::ClearValue{vk::ClearValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}}};

    return colorAttachmentInfo;
}

void star::core::renderer::SwapChainRenderer::recordCommandBuffer(vk::CommandBuffer &commandBuffer,
                                                                  const int &frameInFlightIndex)
{
    // transition image layout
    vk::ImageMemoryBarrier setupBarrier{};
    setupBarrier.sType = vk::StructureType::eImageMemoryBarrier;
    setupBarrier.oldLayout = vk::ImageLayout::ePresentSrcKHR;
    setupBarrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
    setupBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    setupBarrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    setupBarrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    setupBarrier.srcAccessMask = vk::AccessFlagBits::eNone;
    setupBarrier.dstAccessMask = vk::AccessFlagBits::eShaderWrite;
    setupBarrier.subresourceRange.baseMipLevel = 0;
    setupBarrier.subresourceRange.levelCount = 1;
    setupBarrier.subresourceRange.baseArrayLayer = 0;
    setupBarrier.subresourceRange.layerCount = 1;
    setupBarrier.image = this->renderToImages[this->currentSwapChainImageIndex]->getVulkanImage();

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eVertexShader, {},
                                  {}, nullptr, setupBarrier);

    ////for presentation will need to change layout of the image to presentation
    vk::ImageMemoryBarrier presentBarrier{};
    presentBarrier.sType = vk::StructureType::eImageMemoryBarrier;
    presentBarrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
    presentBarrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
    presentBarrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    presentBarrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    presentBarrier.image = this->renderToImages[this->currentSwapChainImageIndex]->getVulkanImage();
    presentBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    presentBarrier.subresourceRange.baseMipLevel = 0;
    presentBarrier.subresourceRange.levelCount = 1;
    presentBarrier.subresourceRange.baseArrayLayer = 0;
    presentBarrier.subresourceRange.layerCount = 1;

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eFragmentShader,
                                  vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, {}, nullptr, presentBarrier);

    this->Renderer::recordCommandBuffer(commandBuffer, frameInFlightIndex);
}

std::vector<vk::Semaphore> star::core::renderer::SwapChainRenderer::CreateSemaphores(
    star::core::device::DeviceContext &device, const int &numToCreate)
{
    std::vector<vk::Semaphore> semaphores = std::vector<vk::Semaphore>(numToCreate);

    vk::SemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = vk::StructureType::eSemaphoreCreateInfo;

    for (int i = 0; i < numToCreate; i++)
    {
        semaphores[i] = device.getDevice().getVulkanDevice().createSemaphore(semaphoreInfo);

        if (!semaphores[i])
        {
            throw std::runtime_error("failed to create semaphores for a frame");
        }
    }

    return semaphores;
}

void star::core::renderer::SwapChainRenderer::createFences()
{
    // note: fence creation can be rolled into semaphore creation. Seperated for understanding
    inFlightFences.resize(this->numFramesInFlight);

    vk::FenceCreateInfo fenceInfo{};
    fenceInfo.sType = vk::StructureType::eFenceCreateInfo;

    // create the fence in a signaled state
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    for (size_t i = 0; i < this->numFramesInFlight; i++)
    {
        this->inFlightFences[i] = this->device.getDevice().getVulkanDevice().createFence(fenceInfo);
        if (!this->inFlightFences[i])
        {
            throw std::runtime_error("failed to create fence object for a frame");
        }
    }
}

void star::core::renderer::SwapChainRenderer::createFenceImageTracking()
{
    // note: just like createFences() this too can be wrapped into semaphore creation. Seperated for understanding.

    // need to ensure the frame that is going to be drawn to, is the one linked to the expected fence.
    // If, for any reason, vulkan returns an image out of order, we will be able to handle that with this link
    imagesInFlight.resize(this->renderToImages.size(), VK_NULL_HANDLE);

    // initially, no frame is using any image so this is going to be created without an explicit link
}

void star::core::renderer::SwapChainRenderer::createSwapChain()
{
    // TODO: current implementation requires halting to all rendering when recreating swapchain. Can place old swap
    // chain in oldSwapChain field
    //   in order to prevent this and allow rendering to continue
    auto swapChainSupport = this->device.getSwapchainSupportDetails();
    vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    // how many images should be in the swap chain
    // in order to avoid extra waiting for driver overhead, author of tutorial recommends +1 of the minimum
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    // with this additional +1 make sure not to go over maximum permitted
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo{};
    createInfo.sType = vk::StructureType::eSwapchainCreateInfoKHR;
    createInfo.surface = this->device.getSurface()->getSurface();

    // specify image information for the surface
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1; // 1 unless using 3D display
    createInfo.imageUsage =
        vk::ImageUsageFlagBits::eColorAttachment |
        vk::ImageUsageFlagBits::eTransferSrc; // how are these images going to be used? Color attachment since we are
                                              // rendering to them (can change for postprocessing effects)

    std::vector<uint32_t> queueFamilyIndicies =
        this->device.getDevice().getQueueOwnershipTracker().getAllQueueFamilyIndices();

    if (queueFamilyIndicies.size() > 1)
    {
        /*need to handle how images will be transferred between different queues
         * so we need to draw images on the graphics queue and then submitting them to the presentation queue
         * Two ways of handling this:
         * 1. VK_SHARING_MODE_EXCLUSIVE: an image is owned by one queue family at a time and can be transferred between
         * groups
         * 2. VK_SHARING_MODE_CONCURRENT: images can be used across queue families without explicit ownership
         */
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = queueFamilyIndicies.size();
        createInfo.pQueueFamilyIndices = queueFamilyIndicies.data();
    }
    else
    {
        // same family is used for graphics and presenting
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0;     // optional
        createInfo.pQueueFamilyIndices = nullptr; // optional
    }

    // can specify certain transforms if they are supported (like 90 degree clockwise rotation)
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;

    // what present mode is going to be used
    createInfo.presentMode = presentMode;
    // if clipped is set to true, we dont care about color of pixels that arent in sight -- best performance to enable
    // this
    createInfo.clipped = VK_TRUE;

    // for now, only assume we are making one swapchain
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    this->swapChain = this->device.getDevice().getVulkanDevice().createSwapchainKHR(createInfo);

    this->swapChainExtent = std::make_unique<vk::Extent2D>(extent);
}

void star::core::renderer::SwapChainRenderer::recreateSwapChain()
{
    int width = 0, height = 0;

    // check for window minimization and wait for window size to no longer be 0
    glfwGetFramebufferSize(this->window.getGLFWwindow(), &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(this->window.getGLFWwindow(), &width, &height);
        glfwWaitEvents();
    }
    // wait for device to finish any current actions
    vkDeviceWaitIdle(this->device.getDevice().getVulkanDevice());

    cleanupSwapChain();

    // create swap chain itself
    createSwapChain();
}

void star::core::renderer::SwapChainRenderer::cleanupSwapChain()
{
    for (auto &image : this->renderToImages)
    {
        image.release();
    }
    for (auto &image : this->renderToDepthImages)
    {
        image.release();
    }

    this->device.getDevice().getVulkanDevice().destroySwapchainKHR(this->swapChain);
}
