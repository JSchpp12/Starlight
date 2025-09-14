#include "renderer/Renderer.hpp"

#include "ManagerController_RenderResource_GlobalInfo.hpp"
#include "ManagerController_RenderResource_LightInfo.hpp"
#include "ManagerController_RenderResource_LightList.hpp"
#include "ManagerRenderResource.hpp"
#include "exception/NeedsPrepared.hpp"


namespace star::core::renderer
{

void Renderer::prepRender(core::device::DeviceContext &device, const vk::Extent2D &swapChainExtent,
                       const int &numFramesInFlight)
    {
        m_commandBuffer = device.getManagerCommandBuffer().submit(getCommandBufferRequest());

        this->swapChainExtent = std::make_unique<vk::Extent2D>(swapChainExtent);

        auto globalBuilder = manualCreateDescriptors(device, numFramesInFlight);
        this->renderToImages = createRenderToImages(device, numFramesInFlight);
        assert(this->renderToImages.size() > 0 && "Need at least 1 image for rendering");
        this->renderToDepthImages = createRenderToDepthImages(device, numFramesInFlight);
        assert(this->renderToDepthImages.size() > 0 && "Need at least 1 depth image for rendering");
        createRenderingGroups(device, swapChainExtent, numFramesInFlight, globalBuilder);
}

void Renderer::update(core::device::DeviceContext &device, const uint8_t &frameInFlightIndex)
{
}

void Renderer::frameUpdate(core::device::DeviceContext &device)
{
    for (size_t i = 0; i < m_objects.size(); i++)
    {
        m_objects[i]->frameUpdate(device);
        // try{
        // m_objects[i]->frameUpdate(device);
        // }catch (const core::exception::NeedsPrepared &err){
        //     m_objects[i]->prepDraw()
        // }
    }
}

void Renderer::initBuffers(core::device::DeviceContext &context, const uint8_t &numFramesInFlight)
{
    m_lightListBuffers.resize(numFramesInFlight);
    m_lightInfoBuffers.resize(numFramesInFlight);

    for (uint16_t i = 0; i < numFramesInFlight; i++)
    {
        m_lightListBuffers[i] = ManagerRenderResource::addRequest(
            context.getDeviceID(), std::make_unique<star::ManagerController::RenderResource::LightList>(i, m_lights));

        m_lightInfoBuffers[i] = ManagerRenderResource::addRequest(
            context.getDeviceID(), std::make_unique<star::ManagerController::RenderResource::LightInfo>(i, m_lights));
    }
}

void Renderer::initBuffers(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                           std::shared_ptr<StarCamera> camera)
{
    initBuffers(context, numFramesInFlight);

    m_cameraInfoBuffers.resize(numFramesInFlight);

    for (uint8_t i = 0; i < numFramesInFlight; i++)
    {
        m_cameraInfoBuffers[i] = ManagerRenderResource::addRequest(
            context.getDeviceID(), std::make_unique<star::ManagerController::RenderResource::GlobalInfo>(i, camera));
    }
}

std::vector<std::unique_ptr<star::StarTextures::Texture>> Renderer::createRenderToImages(
    star::core::device::DeviceContext &device, const int &numFramesInFlight)
{
    std::vector<std::unique_ptr<StarTextures::Texture>> newRenderToImages =
        std::vector<std::unique_ptr<StarTextures::Texture>>();

    std::vector<uint32_t> indices = std::vector<uint32_t>();
    indices.push_back(device.getDevice().getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex());
    if (device.getDevice().getDefaultQueue(star::Queue_Type::Tpresent).getParentQueueFamilyIndex() != indices.back())
    {
        indices.push_back(device.getDevice().getDefaultQueue(star::Queue_Type::Tpresent).getParentQueueFamilyIndex());
    }

    vk::Format format = getColorAttachmentFormat(device);
    uint32_t numIndices; 
    CastHelpers::SafeCast<size_t, uint32_t>(indices.size(), numIndices); 

    auto builder =
        star::StarTextures::Texture::Builder(device.getDevice().getVulkanDevice(),
                                             device.getDevice().getAllocator().get())
            .setCreateInfo(
                Allocator::AllocationBuilder()
                    .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                    .setUsage(VMA_MEMORY_USAGE_GPU_ONLY)
                    .build(),
                vk::ImageCreateInfo()
                    .setExtent(vk::Extent3D()
                                   .setWidth(static_cast<int>(this->swapChainExtent->width))
                                   .setHeight(static_cast<int>(this->swapChainExtent->height))
                                   .setDepth(1))
                    .setPQueueFamilyIndices(indices.data())
                    .setQueueFamilyIndexCount(numIndices)
                    .setSharingMode(indices.size() == 1 ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent)
                    .setUsage(vk::ImageUsageFlagBits::eColorAttachment)
                    .setImageType(vk::ImageType::e2D)
                    .setMipLevels(1)
                    .setTiling(vk::ImageTiling::eOptimal)
                    .setInitialLayout(vk::ImageLayout::eUndefined)
                    .setSamples(vk::SampleCountFlagBits::e1),
                "OffscreenRenderToImages")
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

    for (int i = 0; i < numFramesInFlight; i++)
    {
        newRenderToImages.emplace_back(builder.build());

        auto oneTimeSetup = device.getDevice().beginSingleTimeCommands();

        vk::ImageMemoryBarrier barrier{};
        barrier.sType = vk::StructureType::eImageMemoryBarrier;
        barrier.oldLayout = vk::ImageLayout::eUndefined;
        barrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

        barrier.image = newRenderToImages.back()->getVulkanImage();
        barrier.srcAccessMask = vk::AccessFlagBits::eNone;
        barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

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

std::vector<std::unique_ptr<star::StarTextures::Texture>> star::core::renderer::Renderer::createRenderToDepthImages(
    core::device::DeviceContext &device, const int &numFramesInFlight)
{
    std::vector<std::unique_ptr<StarTextures::Texture>> newRenderToImages =
        std::vector<std::unique_ptr<StarTextures::Texture>>();

    const vk::Format depthFormat = getDepthAttachmentFormat(device);

    std::vector<uint32_t> indices = std::vector<uint32_t>();
    indices.push_back(device.getDevice().getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex());
    if (device.getDevice().getDefaultQueue(star::Queue_Type::Tpresent).getParentQueueFamilyIndex() != indices.back())
    {
        indices.push_back(device.getDevice().getDefaultQueue(star::Queue_Type::Tpresent).getParentQueueFamilyIndex());
    }

    auto builder =
        star::StarTextures::Texture::Builder(device.getDevice().getVulkanDevice(),
                                             device.getDevice().getAllocator().get())
            .setCreateInfo(
                Allocator::AllocationBuilder()
                    .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                    .setUsage(VMA_MEMORY_USAGE_GPU_ONLY)
                    .build(),
                vk::ImageCreateInfo()
                    .setExtent(vk::Extent3D()
                                   .setWidth(static_cast<int>(this->swapChainExtent->width))
                                   .setHeight(static_cast<int>(this->swapChainExtent->height))
                                   .setDepth(1))
                    .setArrayLayers(1)
                    .setQueueFamilyIndexCount(indices.size())
                    .setPQueueFamilyIndices(indices.data())
                    .setSharingMode(indices.size() == 1 ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent)
                    .setQueueFamilyIndexCount(1)
                    .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
                    .setImageType(vk::ImageType::e2D)
                    .setMipLevels(1)
                    .setTiling(vk::ImageTiling::eOptimal)
                    .setInitialLayout(vk::ImageLayout::eUndefined)
                    .setSamples(vk::SampleCountFlagBits::e1),
                "OffscreenRenderToImagesDepth")
            .setBaseFormat(depthFormat)
            .addViewInfo(vk::ImageViewCreateInfo()
                             .setViewType(vk::ImageViewType::e2D)
                             .setFormat(depthFormat)
                             .setSubresourceRange(vk::ImageSubresourceRange()
                                                      .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                                      .setBaseArrayLayer(0)
                                                      .setLayerCount(1)
                                                      .setBaseMipLevel(0)
                                                      .setLevelCount(1)));

    for (int i = 0; i < numFramesInFlight; i++)
    {
        newRenderToImages.emplace_back(builder.build());

        auto oneTimeSetup = device.getDevice().beginSingleTimeCommands();

        vk::ImageMemoryBarrier barrier{};
        barrier.sType = vk::StructureType::eImageMemoryBarrier;
        barrier.oldLayout = vk::ImageLayout::eUndefined;
        barrier.newLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

        barrier.image = newRenderToImages.back()->getVulkanImage();
        barrier.srcAccessMask = vk::AccessFlagBits::eNone;
        barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;

        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        barrier.subresourceRange.baseMipLevel = 0; // image does not have any mipmap levels
        barrier.subresourceRange.levelCount = 1;   // image is not an array
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        oneTimeSetup->buffer().pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,         // which pipeline stages should occurr before barrier
            vk::PipelineStageFlagBits::eLateFragmentTests, // pipeline stage in which operations will wait on the
                                                           // barrier
            {}, {}, nullptr, barrier);

        device.getDevice().endSingleTimeCommands(std::move(oneTimeSetup));
    }

    return newRenderToImages;
}

void Renderer::createRenderingGroups(core::device::DeviceContext &device, const vk::Extent2D &swapChainExtent,
                                     const int &numFramesInFlight, star::StarShaderInfo::Builder builder)
{
    for (size_t i = 0; i < m_objects.size(); i++)
    {
        // check if the object is compatible with any render groups
        StarRenderGroup *match = nullptr;

        // if it is not, create a new render group
        for (size_t j = 0; j < this->renderGroups.size(); j++)
        {
            if (this->renderGroups[j]->isObjectCompatible(*m_objects[i]))
            {
                match = this->renderGroups[j].get();
                break;
            }
        }

        if (match != nullptr)
        {
            match->addObject(*m_objects[i]);
        }
        else
        {
            // create a new one and add object
            this->renderGroups.push_back(std::unique_ptr<StarRenderGroup>(
                new StarRenderGroup(device, numFramesInFlight, swapChainExtent, *m_objects[i])));
        }
    }

    // init all groups
    for (auto &group : this->renderGroups)
    {
        RenderingTargetInfo renderInfo =
            RenderingTargetInfo({this->getColorAttachmentFormat(device)}, this->getDepthAttachmentFormat(device));
        group->init(builder, renderInfo);
    }
}

vk::ImageView Renderer::createImageView(star::core::device::DeviceContext &device, vk::Image image, vk::Format format,
                                        vk::ImageAspectFlags aspectFlags)
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

    vk::ImageView imageView = device.getDevice().getVulkanDevice().createImageView(viewInfo);

    if (!imageView)
    {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

star::StarShaderInfo::Builder Renderer::manualCreateDescriptors(star::core::device::DeviceContext &device,
                                                                const int &numFramesInFlight)
{
    this->globalSetLayout = createGlobalDescriptorSetLayout(device, numFramesInFlight);
    auto globalBuilder = StarShaderInfo::Builder(device.getDeviceID(), device.getDevice(), numFramesInFlight)
                             .addSetLayout(this->globalSetLayout);

    assert(m_cameraInfoBuffers.size() == numFramesInFlight && m_lightInfoBuffers.size() == numFramesInFlight &&
           m_lightListBuffers.size() == numFramesInFlight && "Shader info buffers not properly initialized.");
    for (int i = 0; i < numFramesInFlight; i++)
    {
        globalBuilder.startOnFrameIndex(i)
            .startSet()
            .add(m_cameraInfoBuffers[i], false)
            .add(m_lightInfoBuffers[i], false)
            .add(m_lightListBuffers[i], false);
    }

    return globalBuilder;
}

std::shared_ptr<star::StarDescriptorSetLayout> Renderer::createGlobalDescriptorSetLayout(
    device::DeviceContext &context, const uint8_t &numFramesInFlight)
{
    return StarDescriptorSetLayout::Builder(context.getDevice())
        .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eAll)
        .addBinding(1, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eAll)
        .addBinding(2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eAll)
        .build();
}

void Renderer::createImage(star::core::device::DeviceContext &device, uint32_t width, uint32_t height,
                           vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                           vk::MemoryPropertyFlags properties, vk::Image &image, VmaAllocation &imageMemory)
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
    allocInfo.flags =
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT & VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    allocInfo.requiredFlags = (VkMemoryPropertyFlags)properties;

    vmaCreateImage(device.getDevice().getAllocator().get(), (VkImageCreateInfo *)&imageInfo, &allocInfo,
                   (VkImage *)&image, &imageMemory, nullptr);
}

void Renderer::initResources(core::device::DeviceContext &device, const int &numFramesInFlight,
                             const vk::Extent2D &screensize)
{
    this->prepRender(device, screensize, numFramesInFlight);
}

void Renderer::destroyResources(core::device::DeviceContext &device)
{
    for (auto &image : this->renderToImages)
    {
        image.reset();
    }

    for (auto &image : this->renderToDepthImages)
    {
        image.reset();
    }
}

vk::Format Renderer::getColorAttachmentFormat(star::core::device::DeviceContext &device) const
{
    vk::Format selectedFormat = vk::Format();

    if (!device.getDevice().findSupportedFormat({vk::Format::eR8G8B8A8Srgb}, vk::ImageTiling::eOptimal,
                                                vk::FormatFeatureFlagBits::eColorAttachment, selectedFormat))
    {
        throw std::runtime_error("Failed to find supported color format");
    }
    return selectedFormat;
}

vk::Format Renderer::getDepthAttachmentFormat(star::core::device::DeviceContext &device) const
{
    vk::Format selectedFormat = vk::Format();
    if (!device.getDevice().findSupportedFormat(
            {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
            vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment, selectedFormat))
    {
        throw std::runtime_error("Failed to find supported depth format");
    }

    return selectedFormat;
}

std::vector<std::pair<vk::DescriptorType, const int>> Renderer::getDescriptorRequests(const int &numFramesInFlight)
{
    return std::vector<std::pair<vk::DescriptorType, const int>>{
        std::pair<vk::DescriptorType, const int>(vk::DescriptorType::eUniformBuffer, numFramesInFlight * 2),
        std::pair<vk::DescriptorType, const int>(vk::DescriptorType::eStorageBuffer, numFramesInFlight)};
}

void Renderer::createDescriptors(star::core::device::DeviceContext &device, const int &numFramesInFlight)
{
}

void Renderer::recordCommandBuffer(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex)
{
    vk::Viewport viewport = this->prepareRenderingViewport();
    commandBuffer.setViewport(0, viewport);

    recordPreRenderingCalls(commandBuffer, frameInFlightIndex);

    {
        // dynamic rendering used...so dont need all that extra stuff
        vk::RenderingAttachmentInfo colorAttachmentInfo =
            prepareDynamicRenderingInfoColorAttachment(frameInFlightIndex);
        vk::RenderingAttachmentInfo depthAttachmentInfo =
            prepareDynamicRenderingInfoDepthAttachment(frameInFlightIndex);

        auto renderArea = vk::Rect2D{vk::Offset2D{}, *this->swapChainExtent};
        vk::RenderingInfoKHR renderInfo{};
        renderInfo.renderArea = renderArea;
        renderInfo.layerCount = 1;
        renderInfo.pDepthAttachment = &depthAttachmentInfo;
        renderInfo.pColorAttachments = &colorAttachmentInfo;
        renderInfo.colorAttachmentCount = 1;
        commandBuffer.beginRendering(renderInfo);
    }

    recordRenderingCalls(commandBuffer, frameInFlightIndex);

    commandBuffer.endRendering();

    recordPostRenderingCalls(commandBuffer, frameInFlightIndex);
}

vk::RenderingAttachmentInfo star::core::renderer::Renderer::prepareDynamicRenderingInfoColorAttachment(
    const int &frameInFlightIndex)
{
    vk::RenderingAttachmentInfoKHR colorAttachmentInfo{};
    colorAttachmentInfo.imageView = this->renderToImages[frameInFlightIndex]->getImageView();
    colorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachmentInfo.clearValue = vk::ClearValue{vk::ClearValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}}};

    return colorAttachmentInfo;
}

vk::RenderingAttachmentInfo star::core::renderer::Renderer::prepareDynamicRenderingInfoDepthAttachment(
    const int &frameInFlightIndex)
{
    vk::RenderingAttachmentInfoKHR depthAttachmentInfo{};
    depthAttachmentInfo.imageView = this->renderToDepthImages[frameInFlightIndex]->getImageView();
    depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depthAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachmentInfo.clearValue = vk::ClearValue{vk::ClearDepthStencilValue{1.0f}};

    return depthAttachmentInfo;
}

vk::Viewport Renderer::prepareRenderingViewport()
{
    vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)this->swapChainExtent->width;
    viewport.height = (float)this->swapChainExtent->height;
    // Specify values range of depth values to use for the framebuffer. If not doing anything special, leave at default
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    return viewport;
}

void Renderer::recordPreRenderingCalls(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex)
{
    for (auto &group : this->renderGroups)
    {
        group->recordPreRenderPassCommands(commandBuffer, frameInFlightIndex);
    }
}

void Renderer::recordPostRenderingCalls(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex)
{
    for (auto &group : this->renderGroups)
    {
        group->recordPostRenderPassCommands(commandBuffer, frameInFlightIndex);
    }
}

void Renderer::recordRenderingCalls(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex)
{
    for (auto &group : this->renderGroups)
    {
        group->recordRenderPassCommands(commandBuffer, frameInFlightIndex);
    }
}

void Renderer::prepareForSubmission(const int &frameIndexToBeDrawn)
{
    for (size_t i = 0; i < m_objects.size(); i++)
    {
        m_objects[i]->prepDraw(frameIndexToBeDrawn);
    }
}

} // namespace star::core::renderer