#include "renderer/Renderer.hpp"

#include "ManagerController_RenderResource_GlobalInfo.hpp"
#include "ManagerController_RenderResource_LightInfo.hpp"
#include "ManagerController_RenderResource_LightList.hpp"
#include "ManagerRenderResource.hpp"

#include <algorithm>

namespace star::core::renderer
{

void Renderer::prepRender(core::device::DeviceContext &device, const vk::Extent2D &swapChainExtent,
                          const uint8_t &numFramesInFlight)
{
    m_infoManagerLightData->prepRender(device, numFramesInFlight);
    m_infoManagerLightList->prepRender(device, numFramesInFlight);

    if (m_infoManagerCamera)
    {
        m_infoManagerCamera->prepRender(device, numFramesInFlight);
    }
    m_commandBuffer = device.getManagerCommandBuffer().submit(getCommandBufferRequest(), device.getCurrentFrameIndex());

    this->swapChainExtent = std::make_unique<vk::Extent2D>(swapChainExtent);

    auto rendererDescriptors = manualCreateDescriptors(device, numFramesInFlight);
    this->renderToImages = createRenderToImages(device, numFramesInFlight);
    assert(this->renderToImages.size() > 0 && "Need at least 1 image for rendering");
    this->renderToDepthImages = createRenderToDepthImages(device, numFramesInFlight);
    assert(this->renderToDepthImages.size() > 0 && "Need at least 1 depth image for rendering");
    RenderingTargetInfo renderInfo =
        RenderingTargetInfo({this->getColorAttachmentFormat(device)}, this->getDepthAttachmentFormat(device));

    for (auto &group : renderGroups)
    {
        group->prepRender(device, device.getRenderingSurface().getResolution(), numFramesInFlight, rendererDescriptors,
                          renderInfo);
    }
}

void Renderer::cleanupRender(core::device::DeviceContext &context)
{
    for (auto &group : this->renderGroups)
    {
        group->cleanupRender(context);
    }
}

void Renderer::frameUpdate(core::device::DeviceContext &device, const uint8_t &frameInFlightIndex)
{
    m_renderingContext = core::renderer::RenderingContext{};

    updateDependentData(device, frameInFlightIndex);
    updateRenderingGroups(device, frameInFlightIndex);
}

void Renderer::initBuffers(core::device::DeviceContext &context, const uint8_t &numFramesInFlight)
{
    m_infoManagerLightData =
        std::make_shared<ManagerController::RenderResource::LightInfo>(numFramesInFlight, m_lights);
    m_infoManagerLightList =
        std::make_shared<ManagerController::RenderResource::LightList>(numFramesInFlight, m_lights);
}

void Renderer::initBuffers(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                           std::shared_ptr<StarCamera> camera)
{
    initBuffers(context, numFramesInFlight);

    m_infoManagerCamera = std::make_unique<ManagerController::RenderResource::GlobalInfo>(numFramesInFlight, camera);
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
                                     std::vector<std::shared_ptr<StarObject>> objects)
{
    for (size_t i = 0; i < objects.size(); i++)
    {
        // check if the object is compatible with any render groups
        StarRenderGroup *match = nullptr;

        // if it is not, create a new render group
        for (size_t j = 0; j < this->renderGroups.size(); j++)
        {
            if (this->renderGroups[j]->isObjectCompatible(*objects[i]))
            {
                match = this->renderGroups[j].get();
                break;
            }
        }

        if (match != nullptr)
        {
            match->addObject(objects[i]);
        }
        else
        {
            // create a new one and add object
            this->renderGroups.push_back(std::unique_ptr<StarRenderGroup>(new StarRenderGroup(device, objects[i])));
        }
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

star::StarShaderInfo::Builder Renderer::manualCreateDescriptors(star::core::device::DeviceContext &context,
                                                                const int &numFramesInFlight)
{
    assert(m_infoManagerCamera &&
           "Camera info does not always need to exist. But it should. Hitting this means a change is needed");

    this->globalSetLayout = createGlobalDescriptorSetLayout(context, numFramesInFlight);
    auto globalBuilder = StarShaderInfo::Builder(context.getDeviceID(), context.getDevice(), numFramesInFlight)
                             .addSetLayout(this->globalSetLayout);
    for (int i = 0; i < numFramesInFlight; i++)
    {
        const auto &lightInfoHandle = m_infoManagerLightData->getHandle(i);
        const auto &lightListHandle = m_infoManagerLightList->getHandle(i);
        const auto &cameraHandle = m_infoManagerCamera->getHandle(i);

        globalBuilder.startOnFrameIndex(i)
            .startSet()
            .add(cameraHandle, &context.getManagerRenderResource()
                                    .get<StarBuffers::Buffer>(context.getDeviceID(), cameraHandle)
                                    ->resourceSemaphore)
            .add(lightInfoHandle, &context.getManagerRenderResource()
                                       .get<StarBuffers::Buffer>(context.getDeviceID(), lightInfoHandle)
                                       ->resourceSemaphore)
            .add(lightListHandle, &context.getManagerRenderResource()
                                       .get<StarBuffers::Buffer>(context.getDeviceID(), lightListHandle)
                                       ->resourceSemaphore);
    }

    return globalBuilder;
}

std::shared_ptr<star::StarDescriptorSetLayout> Renderer::createGlobalDescriptorSetLayout(
    device::DeviceContext &context, const uint8_t &numFramesInFlight)
{
    return StarDescriptorSetLayout::Builder()
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
    // this->prepRender(device, screensize, numFramesInFlight);
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

void Renderer::updateDependentData(star::core::device::DeviceContext &context, const uint8_t &frameInFlightIndex)
{
    auto dataSemaphore = vk::Semaphore();

    auto &record = context.getManagerCommandBuffer().m_manager.get(m_commandBuffer);

    if (m_infoManagerCamera && m_infoManagerCamera->submitUpdateIfNeeded(context, frameInFlightIndex, dataSemaphore))
    {
        record.oneTimeWaitSemaphoreInfo.insert(
            m_infoManagerCamera->getHandle(frameInFlightIndex), std::move(dataSemaphore),
            vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader);

        m_renderingContext.addBufferToRenderingContext(context, m_infoManagerCamera->getHandle(frameInFlightIndex));
    }

    if (m_infoManagerLightData->submitUpdateIfNeeded(context, frameInFlightIndex, dataSemaphore))
    {
        record.oneTimeWaitSemaphoreInfo.insert(m_infoManagerLightData->getHandle(frameInFlightIndex),
                                               std::move(dataSemaphore), vk::PipelineStageFlagBits::eFragmentShader);

        m_renderingContext.addBufferToRenderingContext(context, m_infoManagerLightData->getHandle(frameInFlightIndex)); 
    }

    if (m_infoManagerLightList->submitUpdateIfNeeded(context, frameInFlightIndex, dataSemaphore))
    {
        record.oneTimeWaitSemaphoreInfo.insert(m_infoManagerLightList->getHandle(frameInFlightIndex),
                                               std::move(dataSemaphore), vk::PipelineStageFlagBits::eFragmentShader);
        
        m_renderingContext.addBufferToRenderingContext(context, m_infoManagerLightList->getHandle(frameInFlightIndex));
    }
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

void Renderer::recordCommandBuffer(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                   const uint64_t &frameIndex)
{
    vk::Viewport viewport = this->prepareRenderingViewport();
    commandBuffer.setViewport(0, viewport);

    recordPreRenderPassCommands(commandBuffer, frameInFlightIndex, frameIndex);

    recordCommandBufferDependencies(commandBuffer, frameInFlightIndex, frameIndex);

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

    recordRenderingCalls(commandBuffer, frameInFlightIndex, frameIndex);

    commandBuffer.endRendering();

    recordPostRenderingCalls(commandBuffer, frameInFlightIndex);
}

void Renderer::recordCommandBufferDependencies(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                               const uint64_t &frameIndex)
{
    auto memoryBarriers = getMemoryBarriersForThisFrame(frameInFlightIndex, frameIndex);

    commandBuffer.pipelineBarrier2(vk::DependencyInfo()
                                       .setBufferMemoryBarrierCount(memoryBarriers.size())
                                       .setPBufferMemoryBarriers(memoryBarriers.data()));
}

std::vector<vk::BufferMemoryBarrier2> Renderer::getMemoryBarriersForThisFrame(const uint8_t &frameInFlightIndex,
                                                                              const uint64_t &frameIndex)
{
    auto barriers = std::vector<vk::BufferMemoryBarrier2>();

    if (m_infoManagerCamera->willBeUpdatedThisFrame(frameIndex, frameInFlightIndex))
    {
        barriers.emplace_back(
            vk::BufferMemoryBarrier2()
                .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
                .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
                .setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader |
                                 vk::PipelineStageFlagBits2::eVertexShader)
                .setDstAccessMask(vk::AccessFlagBits2::eUniformRead | vk::AccessFlagBits2::eShaderRead)
                .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                .setBuffer(
                    m_renderingContext.bufferTransferRecords.get(m_infoManagerCamera->getHandle(frameInFlightIndex)))
                .setSize(vk::WholeSize));
    }

    if (m_infoManagerLightData->willBeUpdatedThisFrame(frameIndex, frameInFlightIndex))
    {
        barriers.emplace_back(
            vk::BufferMemoryBarrier2()
                .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
                .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
                .setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader |
                                 vk::PipelineStageFlagBits2::eVertexShader)
                .setDstAccessMask(vk::AccessFlagBits2::eUniformRead | vk::AccessFlagBits2::eShaderRead)
                .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                .setBuffer(
                    m_renderingContext.bufferTransferRecords.get(m_infoManagerLightData->getHandle(frameInFlightIndex)))
                .setSize(vk::WholeSize));
    }

    if (m_infoManagerLightList->willBeUpdatedThisFrame(frameIndex, frameInFlightIndex))
    {
        barriers.emplace_back(vk::BufferMemoryBarrier2()
                                  .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
                                  .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
                                  .setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader |
                                                   vk::PipelineStageFlagBits2::eVertexShader)
                                  .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                                  .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                                  .setBuffer(m_renderingContext.bufferTransferRecords.get(
                                      m_infoManagerLightList->getHandle(frameInFlightIndex)))
                                  .setSize(vk::WholeSize));
    }

    return barriers;
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

void Renderer::recordPreRenderPassCommands(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                           const uint64_t &frameIndex)
{
    for (auto &group : this->renderGroups)
    {
        group->recordPreRenderPassCommands(commandBuffer, frameInFlightIndex, frameIndex);
    }
}

void Renderer::recordPostRenderingCalls(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex)
{
    for (auto &group : this->renderGroups)
    {
        group->recordPostRenderPassCommands(commandBuffer, frameInFlightIndex);
    }
}

void Renderer::recordRenderingCalls(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                    const uint64_t &frameIndex)
{
    for (auto &group : this->renderGroups)
    {
        group->recordRenderPassCommands(commandBuffer, frameInFlightIndex, frameIndex);
    }
}

void Renderer::updateRenderingGroups(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex)
{
    for (auto &group : renderGroups)
    {
        group->frameUpdate(context, frameInFlightIndex, m_commandBuffer);
    }
}

} // namespace star::core::renderer