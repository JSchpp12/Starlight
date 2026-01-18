#include "renderer/DefaultRenderer.hpp"

#include "ManagerController_RenderResource_GlobalInfo.hpp"
#include "ManagerController_RenderResource_LightInfo.hpp"
#include "ManagerController_RenderResource_LightList.hpp"
#include "ManagerRenderResource.hpp"
#include "core/device/system/event/ManagerRequest.hpp"
#include "core/helper/command_buffer/CommandBufferHelpers.hpp"
#include "core/helper/queue/QueueHelpers.hpp"

#include <star_common/HandleTypeRegistry.hpp>
#include <vma/vk_mem_alloc.h>

#include <algorithm>

namespace star::core::renderer
{

void DefaultRenderer::prepRender(common::IDeviceContext &device)
{
    RendererBase::prepRender(device);

    auto &c = static_cast<core::device::DeviceContext &>(device);
    m_infoManagerLightData->prepRender(c, c.getFrameTracker().getSetup().getNumFramesInFlight());
    m_infoManagerLightList->prepRender(c, c.getFrameTracker().getSetup().getNumFramesInFlight());

    if (m_infoManagerCamera)
    {
        m_infoManagerCamera->prepRender(c, c.getFrameTracker().getSetup().getNumFramesInFlight());
    }

    m_renderingContext.targetResolution = c.getEngineResolution();

    auto rendererDescriptors = manualCreateDescriptors(c, c.getFrameTracker().getSetup().getNumFramesInFlight());
    {
        auto images = createRenderToImages(c, c.getFrameTracker().getSetup().getNumFramesInFlight());
        m_colorFormat = images.front().getBaseFormat();
        m_renderToImages.resize(images.size());

        for (size_t i = 0; i < images.size(); i++)
        {
            void *r = nullptr;
            device.getEventBus().emit(core::device::system::event::ManagerRequest{
                star::common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                    core::device::manager::GetImageEventTypeName),
                core::device::manager::ImageRequest{std::move(images[i])}, m_renderToImages[i], &r});

            assert(r != nullptr);
            auto *result = static_cast<core::device::manager::ImageRecord *>(r);
            m_renderingContext.recordDependentImage.manualInsert(m_renderToImages[i], &result->texture);
        }
    }
    {
        auto images = createRenderToDepthImages(c, c.getFrameTracker().getSetup().getNumFramesInFlight());
        m_depthFormat = images.front().getBaseFormat();
        m_renderToDepthImages.resize(images.size());

        for (size_t i = 0; i < images.size(); i++)
        {
            void *r = nullptr;
            device.getEventBus().emit(core::device::system::event::ManagerRequest{
                star::common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                    core::device::manager::GetImageEventTypeName),
                core::device::manager::ImageRequest{std::move(images[i])}, m_renderToDepthImages[i], &r});

            auto *result = static_cast<core::device::manager::ImageRecord *>(r);
            m_renderingContext.recordDependentImage.manualInsert(m_renderToDepthImages[i], &result->texture);
        }
    }

    RenderingTargetInfo renderInfo =
        RenderingTargetInfo({this->getColorAttachmentFormat(c)}, this->getDepthAttachmentFormat(c));

    for (auto &group : m_renderGroups)
    {
        group.prepRender(c, c.getEngineResolution(), c.getFrameTracker().getSetup().getNumFramesInFlight(),
                         rendererDescriptors, renderInfo);
    }
}

void DefaultRenderer::cleanupRender(common::IDeviceContext &context)
{
    auto &c = static_cast<core::device::DeviceContext &>(context);

    RendererBase::cleanupRender(c);
}

void DefaultRenderer::frameUpdate(common::IDeviceContext &context)
{
    RendererBase::frameUpdate(context);

    auto &c = static_cast<core::device::DeviceContext &>(context);
    size_t i = static_cast<size_t>(c.getFrameTracker().getCurrent().getFrameInFlightIndex());
    m_renderingContext.recordDependentImage.manualInsert(m_renderToImages[i],
                                                         &c.getImageManager().get(m_renderToImages[i])->texture);
    m_renderingContext.recordDependentImage.manualInsert(m_renderToDepthImages[i],
                                                         &c.getImageManager().get(m_renderToDepthImages[i])->texture);

    updateDependentData(c, c.getFrameTracker().getCurrent().getFrameInFlightIndex());
}

void DefaultRenderer::initBuffers(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                                  std::shared_ptr<std::vector<Light>> lights)
{
    m_infoManagerLightData = std::make_shared<ManagerController::RenderResource::LightInfo>(numFramesInFlight, lights);
    m_infoManagerLightList = std::make_shared<ManagerController::RenderResource::LightList>(numFramesInFlight, lights);
}

void DefaultRenderer::initBuffers(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                                  std::shared_ptr<std::vector<Light>> lights, std::shared_ptr<StarCamera> camera)
{
    initBuffers(context, numFramesInFlight, std::move(lights));

    m_infoManagerCamera = std::make_unique<ManagerController::RenderResource::GlobalInfo>(camera);
}

std::vector<star::StarTextures::Texture> DefaultRenderer::createRenderToImages(
    star::core::device::DeviceContext &device, const uint8_t &numFramesInFlight)
{
    std::vector<StarTextures::Texture> newRenderToImages = std::vector<StarTextures::Texture>();

    std::vector<uint32_t> indices = std::vector<uint32_t>();
    indices.push_back(core::helper::GetEngineDefaultQueue(
                          device.getEventBus(), device.getGraphicsManagers().queueManager, star::Queue_Type::Tgraphics)
                          ->getParentQueueFamilyIndex());

    {
        StarQueue *presentQueue = core::helper::GetEngineDefaultQueue(
            device.getEventBus(), device.getGraphicsManagers().queueManager, star::Queue_Type::Tpresent);

        if (presentQueue != nullptr && presentQueue->getParentQueueFamilyIndex() != indices.back())
        {
            indices.push_back(presentQueue->getParentQueueFamilyIndex());
        }
    }

    vk::Format format = getColorAttachmentFormat(device);
    uint32_t numIndices;
    common::helper::SafeCast<size_t, uint32_t>(indices.size(), numIndices);

    int width, height;
    {
        const auto &resolution = device.getEngineResolution();
        common::helper::SafeCast<vk::DeviceSize, int>(resolution.width, width);
        common::helper::SafeCast<vk::DeviceSize, int>(resolution.height, height);
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
                    .setExtent(vk::Extent3D().setWidth(width).setHeight(height).setDepth(1))
                    .setPQueueFamilyIndices(indices.data())
                    .setArrayLayers(1)
                    .setQueueFamilyIndexCount(numIndices)
                    .setSharingMode(indices.size() == 1 ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent)
                    .setUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc)
                    .setImageType(vk::ImageType::e2D)
                    .setMipLevels(1)
                    .setTiling(vk::ImageTiling::eOptimal)
                    .setInitialLayout(vk::ImageLayout::eUndefined)
                    .setSamples(vk::SampleCountFlagBits::e1),
                "RendererColorImage")
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

    auto *singleTimeTargetQueue = core::helper::GetEngineDefaultQueue(
        device.getEventBus(), device.getGraphicsManagers().queueManager, star::Queue_Type::Tgraphics);
    assert(singleTimeTargetQueue != nullptr);

    for (int i = 0; i < numFramesInFlight; i++)
    {
        newRenderToImages.emplace_back(builder.build());
        newRenderToImages.back().setImageLayout(vk::ImageLayout::eColorAttachmentOptimal);

        auto oneTimeSetup = core::helper::BeginSingleTimeCommands(device.getDevice(), device.getEventBus(),
                                                                  device.getManagerCommandBuffer().m_manager,
                                                                  star::Queue_Type::Tgraphics);

        vk::ImageMemoryBarrier barrier{};
        barrier.sType = vk::StructureType::eImageMemoryBarrier;
        barrier.oldLayout = vk::ImageLayout::eUndefined;
        barrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

        barrier.image = newRenderToImages.back().getVulkanImage();
        barrier.srcAccessMask = vk::AccessFlagBits::eNone;
        barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseMipLevel = 0; // image does not have any mipmap levels
        barrier.subresourceRange.levelCount = 1;   // image is not an array
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        oneTimeSetup.buffer().pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,             // which pipeline stages should
                                                               // occurr before barrier
            vk::PipelineStageFlagBits::eColorAttachmentOutput, // pipeline stage in
                                                               // which operations will
                                                               // wait on the barrier
            {}, {}, nullptr, barrier);

        core::helper::EndSingleTimeCommands(*singleTimeTargetQueue, std::move(oneTimeSetup));
    }

    return newRenderToImages;
}

std::vector<star::StarTextures::Texture> star::core::renderer::DefaultRenderer::createRenderToDepthImages(
    core::device::DeviceContext &device, const uint8_t &numFramesInFlight)
{
    std::vector<StarTextures::Texture> newRenderToImages;

    const vk::Format depthFormat = getDepthAttachmentFormat(device);

    std::vector<uint32_t> indices = std::vector<uint32_t>();
    const auto graphicsQueueFamilyIndex =
        core::helper::GetEngineDefaultQueue(device.getEventBus(), device.getGraphicsManagers().queueManager,
                                            star::Queue_Type::Tgraphics)
            ->getParentQueueFamilyIndex();
    indices.push_back(graphicsQueueFamilyIndex);

    auto *presentQueueFamily = core::helper::GetEngineDefaultQueue(
        device.getEventBus(), device.getGraphicsManagers().queueManager, star::Queue_Type::Tpresent);

    if (presentQueueFamily != nullptr && presentQueueFamily->getParentQueueFamilyIndex() != graphicsQueueFamilyIndex)
    {
        indices.push_back(presentQueueFamily->getParentQueueFamilyIndex());
    }

    int width, height;
    {
        const auto &resolution = device.getEngineResolution();
        common::helper::SafeCast<vk::DeviceSize, int>(resolution.width, width);
        common::helper::SafeCast<vk::DeviceSize, int>(resolution.height, height);
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
                    .setExtent(vk::Extent3D().setWidth(width).setHeight(height).setDepth(1))
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
                "RendererDepthImage")
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

    auto *oneTimeTargetQueue = core::helper::GetEngineDefaultQueue(
        device.getEventBus(), device.getGraphicsManagers().queueManager, star::Queue_Type::Tgraphics);
    assert(oneTimeTargetQueue != nullptr);

    for (int i = 0; i < numFramesInFlight; i++)
    {
        newRenderToImages.emplace_back(builder.build());

        auto oneTimeSetup = core::helper::BeginSingleTimeCommands(device.getDevice(), device.getEventBus(),
                                                                  device.getManagerCommandBuffer().m_manager,
                                                                  star::Queue_Type::Tgraphics);

        vk::ImageMemoryBarrier barrier{};
        barrier.sType = vk::StructureType::eImageMemoryBarrier;
        barrier.oldLayout = vk::ImageLayout::eUndefined;
        barrier.newLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

        barrier.image = newRenderToImages.back().getVulkanImage();
        barrier.srcAccessMask = vk::AccessFlagBits::eNone;
        barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;

        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        barrier.subresourceRange.baseMipLevel = 0; // image does not have any mipmap levels
        barrier.subresourceRange.levelCount = 1;   // image is not an array
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        oneTimeSetup.buffer().pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,         // which pipeline stages should occurr before barrier
            vk::PipelineStageFlagBits::eLateFragmentTests, // pipeline stage in which operations will wait on the
                                                           // barrier
            {}, {}, nullptr, barrier);

        core::helper::EndSingleTimeCommands(*oneTimeTargetQueue, std::move(oneTimeSetup));
    }

    return newRenderToImages;
}

vk::ImageView DefaultRenderer::createImageView(star::core::device::DeviceContext &device, vk::Image image,
                                               vk::Format format, vk::ImageAspectFlags aspectFlags)
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

star::StarShaderInfo::Builder DefaultRenderer::manualCreateDescriptors(star::core::device::DeviceContext &context,
                                                                       const uint8_t &numFramesInFlight)
{
    assert(m_infoManagerCamera &&
           "Camera info does not always need to exist. But it should. Hitting this means a change is needed");

    auto defaultPool = Handle{.type = common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                                  core::device::manager::GetDescriptorPoolTypeName),
                              .id = 0};

    this->globalSetLayout = createGlobalDescriptorSetLayout(context, numFramesInFlight);
    auto globalBuilder =
        StarShaderInfo::Builder(context.getDeviceID(), context.getDevice(),
                                *context.getDescriptorPoolManager().get(defaultPool)->pool, numFramesInFlight)
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

std::shared_ptr<star::StarDescriptorSetLayout> DefaultRenderer::createGlobalDescriptorSetLayout(
    device::DeviceContext &context, const uint8_t &numFramesInFlight)
{
    return StarDescriptorSetLayout::Builder()
        .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eAll)
        .addBinding(1, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eAll)
        .addBinding(2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eAll)
        .build();
}

void DefaultRenderer::createImage(star::core::device::DeviceContext &device, uint32_t width, uint32_t height,
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

vk::Format DefaultRenderer::getColorAttachmentFormat(star::core::device::DeviceContext &device) const
{
    vk::Format selectedFormat = vk::Format();

    if (!device.getDevice().findSupportedFormat({vk::Format::eR8G8B8A8Srgb}, vk::ImageTiling::eOptimal,
                                                vk::FormatFeatureFlagBits::eColorAttachment, selectedFormat))
    {
        throw std::runtime_error("Failed to find supported color format");
    }
    return selectedFormat;
}

vk::Format DefaultRenderer::getDepthAttachmentFormat(star::core::device::DeviceContext &device) const
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

void DefaultRenderer::updateDependentData(star::core::device::DeviceContext &context, const uint8_t &frameInFlightIndex)
{
    auto dataSemaphore = vk::Semaphore();

    auto &record = context.getManagerCommandBuffer().m_manager.get(m_commandBuffer);

    if (ownsRenderResourceControllers)
    {
        if (m_infoManagerCamera &&
            m_infoManagerCamera->submitUpdateIfNeeded(context, frameInFlightIndex, dataSemaphore))
        {
            record.oneTimeWaitSemaphoreInfo.insert(
                m_infoManagerCamera->getHandle(frameInFlightIndex), std::move(dataSemaphore),
                vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader);

            m_renderingContext.addBufferToRenderingContext(context, m_infoManagerCamera->getHandle(frameInFlightIndex));
        }

        if (m_infoManagerLightData->submitUpdateIfNeeded(context, frameInFlightIndex, dataSemaphore))
        {
            record.oneTimeWaitSemaphoreInfo.insert(m_infoManagerLightData->getHandle(frameInFlightIndex),
                                                   std::move(dataSemaphore),
                                                   vk::PipelineStageFlagBits::eFragmentShader);

            m_renderingContext.addBufferToRenderingContext(context,
                                                           m_infoManagerLightData->getHandle(frameInFlightIndex));
        }

        if (m_infoManagerLightList->submitUpdateIfNeeded(context, frameInFlightIndex, dataSemaphore))
        {
            record.oneTimeWaitSemaphoreInfo.insert(m_infoManagerLightList->getHandle(frameInFlightIndex),
                                                   std::move(dataSemaphore),
                                                   vk::PipelineStageFlagBits::eFragmentShader);

            m_renderingContext.addBufferToRenderingContext(context,
                                                           m_infoManagerLightList->getHandle(frameInFlightIndex));
        }
    }
}

std::vector<std::pair<vk::DescriptorType, const int>> DefaultRenderer::getDescriptorRequests(
    const int &numFramesInFlight)
{
    return std::vector<std::pair<vk::DescriptorType, const int>>{
        std::pair<vk::DescriptorType, const int>(vk::DescriptorType::eUniformBuffer, numFramesInFlight * 2),
        std::pair<vk::DescriptorType, const int>(vk::DescriptorType::eStorageBuffer, numFramesInFlight)};
}

void DefaultRenderer::createDescriptors(star::core::device::DeviceContext &device, const int &numFramesInFlight)
{
}

void DefaultRenderer::recordCommandBuffer(StarCommandBuffer &commandBuffer, const common::FrameTracker &frameTracker,
                                          const uint64_t &frameIndex)
{
    commandBuffer.begin(frameTracker.getCurrent().getFrameInFlightIndex());

    recordCommands(commandBuffer.buffer(frameTracker.getCurrent().getFrameInFlightIndex()), frameTracker, frameIndex);

    commandBuffer.buffer(frameTracker.getCurrent().getFrameInFlightIndex()).end();
}

void DefaultRenderer::recordCommands(vk::CommandBuffer &commandBuffer, const common::FrameTracker &frameTracker,
                                     const uint64_t &frameIndex)
{
    vk::Viewport viewport = this->prepareRenderingViewport(m_renderingContext.targetResolution);
    commandBuffer.setViewport(0, viewport);

    recordPreRenderPassCommands(commandBuffer, frameTracker.getCurrent().getFrameInFlightIndex(), frameIndex);

    recordCommandBufferDependencies(commandBuffer, frameTracker.getCurrent().getFrameInFlightIndex(), frameIndex);

    {
        // dynamic rendering used...so dont need all that extra stuff
        vk::RenderingAttachmentInfo colorAttachmentInfo = prepareDynamicRenderingInfoColorAttachment(frameTracker);
        vk::RenderingAttachmentInfo depthAttachmentInfo = prepareDynamicRenderingInfoDepthAttachment(frameTracker);

        auto renderArea = vk::Rect2D{vk::Offset2D{}, m_renderingContext.targetResolution};
        vk::RenderingInfoKHR renderInfo{};
        renderInfo.renderArea = renderArea;
        renderInfo.layerCount = 1;
        renderInfo.pDepthAttachment = &depthAttachmentInfo;
        renderInfo.pColorAttachments = &colorAttachmentInfo;
        renderInfo.colorAttachmentCount = 1;
        commandBuffer.beginRendering(renderInfo);
    }

    recordRenderingCalls(commandBuffer, frameTracker.getCurrent().getFrameInFlightIndex(), frameIndex);

    commandBuffer.endRendering();

    recordPostRenderingCalls(commandBuffer, frameTracker.getCurrent().getFrameInFlightIndex());
}

void DefaultRenderer::recordCommandBufferDependencies(vk::CommandBuffer &commandBuffer,
                                                      const uint8_t &frameInFlightIndex, const uint64_t &frameIndex)
{
    auto memoryBarriers = getMemoryBarriersForThisFrame(frameInFlightIndex, frameIndex);

    commandBuffer.pipelineBarrier2(vk::DependencyInfo()
                                       .setBufferMemoryBarrierCount(memoryBarriers.size())
                                       .setPBufferMemoryBarriers(memoryBarriers.data()));
}

std::vector<vk::BufferMemoryBarrier2> DefaultRenderer::getMemoryBarriersForThisFrame(const uint8_t &frameInFlightIndex,
                                                                                     const uint64_t &frameIndex)
{
    auto barriers = std::vector<vk::BufferMemoryBarrier2>();

    if (ownsRenderResourceControllers)
    {
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
                    .setBuffer(m_renderingContext.bufferTransferRecords.get(
                        m_infoManagerCamera->getHandle(frameInFlightIndex)))
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
                    .setBuffer(m_renderingContext.bufferTransferRecords.get(
                        m_infoManagerLightData->getHandle(frameInFlightIndex)))
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
    }

    return barriers;
}

vk::RenderingAttachmentInfo star::core::renderer::DefaultRenderer::prepareDynamicRenderingInfoColorAttachment(
    const common::FrameTracker &frameTracker)
{
    size_t index = static_cast<size_t>(frameTracker.getCurrent().getFrameInFlightIndex());

    vk::RenderingAttachmentInfoKHR colorAttachmentInfo{};
    colorAttachmentInfo.imageView =
        m_renderingContext.recordDependentImage.get(m_renderToImages[index])->getImageView();
    colorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachmentInfo.clearValue = vk::ClearValue{vk::ClearValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}}};

    return colorAttachmentInfo;
}

vk::RenderingAttachmentInfo star::core::renderer::DefaultRenderer::prepareDynamicRenderingInfoDepthAttachment(
    const common::FrameTracker &frameTracker)
{
    size_t index = static_cast<size_t>(frameTracker.getCurrent().getFrameInFlightIndex());

    vk::RenderingAttachmentInfoKHR depthAttachmentInfo{};
    depthAttachmentInfo.imageView =
        m_renderingContext.recordDependentImage.get(m_renderToDepthImages[index])->getImageView();
    depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depthAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachmentInfo.clearValue = vk::ClearValue{vk::ClearDepthStencilValue{1.0f}};

    return depthAttachmentInfo;
}

vk::Viewport DefaultRenderer::prepareRenderingViewport(const vk::Extent2D &resolution)
{
    vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)resolution.width;
    viewport.height = (float)resolution.height;
    // Specify values range of depth values to use for the framebuffer. If not doing anything special, leave at default
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    return viewport;
}

} // namespace star::core::renderer