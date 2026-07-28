#include "core/renderer/RenderTargets.hpp"

#include "Allocator.hpp"
#include "ManagerRenderResource.hpp"
#include "StarTextures/Texture.hpp"
#include "core/device/DeviceContext.hpp"
#include "core/device/system/event/ManagerRequest.hpp"
#include "core/helper/command_buffer/CommandBufferHelpers.hpp"
#include "core/helper/queue/QueueHelpers.hpp"

#include <star_common/HandleTypeRegistry.hpp>
#include <star_common/helper/CastHelpers.hpp>

#include <vma/vk_mem_alloc.h>

#include <cassert>
#include <stdexcept>

namespace star::core::renderer
{
std::vector<Handle> RenderTargets::registerTextures(core::device::DeviceContext &context,
                                                    RenderingContext &renderingContext,
                                                    std::vector<StarTextures::Texture> textures)
{
    std::vector<Handle> handles;
    handles.resize(textures.size());

    for (size_t i = 0; i < textures.size(); i++)
    {
        void *r = nullptr;
        context.getEventBus().emit(core::device::system::event::ManagerRequest{
            star::common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                core::device::manager::GetImageEventTypeName),
            core::device::manager::ImageRequest{std::move(textures[i])}, handles[i], &r});

        assert(r != nullptr);
        auto *result = static_cast<core::device::manager::ImageRecord *>(r);
        renderingContext.recordDependentImage.manualInsert(handles[i], &result->texture);
    }

    return handles;
}

void RenderTargets::frameUpdate(core::device::DeviceContext &context, RenderingContext &renderingContext)
{
    const uint8_t fi = static_cast<uint8_t>(context.frameTracker().getCurrent().getFrameInFlightIndex());

    renderingContext.recordDependentImage.manualInsert(m_colorHandles[fi],
                                                       &context.getImageManager().get(m_colorHandles[fi])->texture);
    renderingContext.recordDependentImage.manualInsert(m_depthHandles[fi],
                                                       &context.getImageManager().get(m_depthHandles[fi])->texture);
}

static void collectGraphicsPresentTransferIndices(core::device::DeviceContext &device, std::vector<uint32_t> &indices)
{
    indices.push_back(core::helper::GetEngineDefaultQueue(
                          device.getEventBus(), device.getGraphicsManagers().queueManager, star::Queue_Type::Tgraphics)
                          ->getParentQueueFamilyIndex());
    {
        auto *q = core::helper::GetEngineDefaultQueue(device.getEventBus(), device.getGraphicsManagers().queueManager,
                                                      star::Queue_Type::Tpresent);
        if (q != nullptr && q->getParentQueueFamilyIndex() != indices.back())
            indices.push_back(q->getParentQueueFamilyIndex());
    }
    {
        auto *q = core::helper::GetEngineDefaultQueue(device.getEventBus(), device.getGraphicsManagers().queueManager,
                                                      star::Queue_Type::Ttransfer);
        if (q != nullptr && q->getParentQueueFamilyIndex() != indices.back())
            indices.push_back(q->getParentQueueFamilyIndex());
    }
}

static void collectGraphicsPresentIndices(core::device::DeviceContext &device, std::vector<uint32_t> &indices)
{
    const auto graphicsQueueFamilyIndex =
        core::helper::GetEngineDefaultQueue(device.getEventBus(), device.getGraphicsManagers().queueManager,
                                            star::Queue_Type::Tgraphics)
            ->getParentQueueFamilyIndex();
    indices.push_back(graphicsQueueFamilyIndex);
    auto *presentQueueFamily = core::helper::GetEngineDefaultQueue(
        device.getEventBus(), device.getGraphicsManagers().queueManager, star::Queue_Type::Tpresent);
    if (presentQueueFamily != nullptr && presentQueueFamily->getParentQueueFamilyIndex() != graphicsQueueFamilyIndex)
        indices.push_back(presentQueueFamily->getParentQueueFamilyIndex());
}

static void engineResolution(core::device::DeviceContext &device, int &width, int &height)
{
    const auto &res = device.getEngineResolution();
    star::common::casts::SafeCast<vk::DeviceSize, int>(res.width, width);
    star::common::casts::SafeCast<vk::DeviceSize, int>(res.height, height);
}

static vk::Format selectFormat(core::device::DeviceContext &device, const std::vector<vk::Format> &candidates,
                               vk::FormatFeatureFlags features)
{
    vk::Format selected = vk::Format();
    if (!device.getDevice().findSupportedFormat(candidates, vk::ImageTiling::eOptimal, features, selected))
        STAR_THROW("RenderTargets: failed to find a supported format for the requested features");
    return selected;
}

// ------------------------------------------------------------------------------------------------
// forPresentation: presented/windowed path. Color shared across graphics+present+transfer and
// transitioned to ColorAttachmentOptimal at creation; depth shared across graphics+present and
// transitioned to DepthStencilAttachmentOptimal. Verbatim of the former DefaultRenderer creation.
// ------------------------------------------------------------------------------------------------
RenderTargets RenderTargets::forPresentation(core::device::DeviceContext &context, RenderingContext &renderingContext)
{
    const uint8_t numFrames =
        static_cast<uint8_t>(context.frameTracker().getSetup().getNumUniqueTargetFramesForFinalization());

    int width, height;
    engineResolution(context, width, height);

    vk::Format colorFormat = vk::Format::eUndefined;
    std::vector<StarTextures::Texture> colorTextures;
    colorTextures.reserve(numFrames);

    {
        std::vector<uint32_t> indices;
        collectGraphicsPresentTransferIndices(context, indices);
        uint32_t numIndices;
        star::common::casts::SafeCast<size_t, uint32_t>(indices.size(), numIndices);
        colorFormat = selectFormat(context, {vk::Format::eR8G8B8A8Srgb}, vk::FormatFeatureFlagBits::eColorAttachment);

        auto builder =
            star::StarTextures::Texture::Builder(context.getDevice().getVulkanDevice(),
                                                 context.getDevice().getAllocator().get())
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
                        .setSharingMode(indices.size() == 1 ? vk::SharingMode::eExclusive
                                                            : vk::SharingMode::eConcurrent)
                        .setUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc)
                        .setImageType(vk::ImageType::e2D)
                        .setMipLevels(1)
                        .setTiling(vk::ImageTiling::eOptimal)
                        .setInitialLayout(vk::ImageLayout::eUndefined)
                        .setSamples(vk::SampleCountFlagBits::e1),
                    "RendererColorImage")
                .setBaseFormat(colorFormat)
                .addViewInfo(vk::ImageViewCreateInfo()
                                 .setViewType(vk::ImageViewType::e2D)
                                 .setFormat(colorFormat)
                                 .setSubresourceRange(vk::ImageSubresourceRange()
                                                          .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                          .setBaseArrayLayer(0)
                                                          .setLayerCount(1)
                                                          .setBaseMipLevel(0)
                                                          .setLevelCount(1)));

        auto *singleTimeTargetQueue = core::helper::GetEngineDefaultQueue(
            context.getEventBus(), context.getGraphicsManagers().queueManager, star::Queue_Type::Tgraphics);
        assert(singleTimeTargetQueue != nullptr);

        for (uint8_t i = 0; i < numFrames; i++)
        {
            colorTextures.emplace_back(builder.build());
            colorTextures.back().setImageLayout(vk::ImageLayout::eColorAttachmentOptimal);

            auto oneTimeSetup = core::helper::BeginSingleTimeCommands(context.getDevice(), context.getEventBus(),
                                                                      context.getManagerCommandBuffer().m_manager,
                                                                      star::Queue_Type::Tgraphics);
            vk::ImageMemoryBarrier barrier{};
            barrier.sType = vk::StructureType::eImageMemoryBarrier;
            barrier.oldLayout = vk::ImageLayout::eUndefined;
            barrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
            barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
            barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
            barrier.image = colorTextures.back().getVulkanImage();
            barrier.srcAccessMask = vk::AccessFlagBits::eNone;
            barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
            barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            oneTimeSetup.buffer().pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                                                  vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, {}, nullptr,
                                                  barrier);
            core::helper::EndSingleTimeCommands(*singleTimeTargetQueue, std::move(oneTimeSetup));
        }
    }

    vk::Format depthFormat = vk::Format::eUndefined;
    std::vector<StarTextures::Texture> depthTextures;
    depthTextures.reserve(numFrames);

    {
        std::vector<uint32_t> indices;
        collectGraphicsPresentIndices(context, indices);
        depthFormat =
            selectFormat(context, {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
                         vk::FormatFeatureFlagBits::eDepthStencilAttachment);

        auto builder =
            star::StarTextures::Texture::Builder(context.getDevice().getVulkanDevice(),
                                                 context.getDevice().getAllocator().get())
                .setCreateInfo(Allocator::AllocationBuilder()
                                   .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                                   .setUsage(VMA_MEMORY_USAGE_GPU_ONLY)
                                   .build(),
                               vk::ImageCreateInfo()
                                   .setExtent(vk::Extent3D().setWidth(width).setHeight(height).setDepth(1))
                                   .setArrayLayers(1)
                                   .setQueueFamilyIndexCount(indices.size())
                                   .setPQueueFamilyIndices(indices.data())
                                   .setSharingMode(indices.size() == 1 ? vk::SharingMode::eExclusive
                                                                       : vk::SharingMode::eConcurrent)
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
            context.getEventBus(), context.getGraphicsManagers().queueManager, star::Queue_Type::Tgraphics);
        assert(oneTimeTargetQueue != nullptr);

        for (uint8_t i = 0; i < numFrames; i++)
        {
            depthTextures.emplace_back(builder.build());
            auto oneTimeSetup = core::helper::BeginSingleTimeCommands(context.getDevice(), context.getEventBus(),
                                                                      context.getManagerCommandBuffer().m_manager,
                                                                      star::Queue_Type::Tgraphics);
            vk::ImageMemoryBarrier barrier{};
            barrier.sType = vk::StructureType::eImageMemoryBarrier;
            barrier.oldLayout = vk::ImageLayout::eUndefined;
            barrier.newLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
            barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
            barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
            barrier.image = depthTextures.back().getVulkanImage();
            barrier.srcAccessMask = vk::AccessFlagBits::eNone;
            barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
            barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            oneTimeSetup.buffer().pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                                                  vk::PipelineStageFlagBits::eLateFragmentTests, {}, {}, nullptr,
                                                  barrier);
            core::helper::EndSingleTimeCommands(*oneTimeTargetQueue, std::move(oneTimeSetup));
        }
    }

    auto colorHandles = registerTextures(context, renderingContext, std::move(colorTextures));
    auto depthHandles = registerTextures(context, renderingContext, std::move(depthTextures));
    return RenderTargets(std::move(colorHandles), colorFormat, std::move(depthHandles), depthFormat);
}

// ------------------------------------------------------------------------------------------------
// forOffscreen: offscreen/compute-read path. Color is exclusive (compute transitions it), no
// creation-time layout transition; depth is exclusive + sampled (sampler attached), no transition.
// Verbatim of the former OffscreenRenderer creation.
// ------------------------------------------------------------------------------------------------
RenderTargets RenderTargets::forOffscreen(core::device::DeviceContext &context, RenderingContext &renderingContext)
{
    const uint8_t numFramesInFlight = static_cast<uint8_t>(context.frameTracker().getSetup().getNumFramesInFlight());

    int width, height;
    engineResolution(context, width, height);
    const auto &props = context.getDevice().getPhysicalDevice().getProperties();

    vk::Format colorFormat = vk::Format::eUndefined;
    std::vector<StarTextures::Texture> colorTextures;
    colorTextures.reserve(numFramesInFlight);
    {
        colorFormat =
            selectFormat(context, {vk::Format::eR8G8B8A8Srgb, vk::Format::eR8G8B8A8Unorm},
                         vk::FormatFeatureFlagBits::eColorAttachment | vk::FormatFeatureFlagBits::eTransferSrc |
                             vk::FormatFeatureFlagBits::eStorageImage);

        auto builder =
            star::StarTextures::Texture::Builder(context.getDevice().getVulkanDevice(),
                                                 context.getDevice().getAllocator().get())
                .setCreateInfo(Allocator::AllocationBuilder()
                                   .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                                   .setUsage(VMA_MEMORY_USAGE_GPU_ONLY)
                                   .build(),
                               vk::ImageCreateInfo()
                                   .setExtent(vk::Extent3D().setWidth(width).setHeight(height).setDepth(1))
                                   .setSharingMode(vk::SharingMode::eExclusive)
                                   .setArrayLayers(1)
                                   .setUsage(vk::ImageUsageFlagBits::eColorAttachment |
                                             vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage)
                                   .setImageType(vk::ImageType::e2D)
                                   .setMipLevels(1)
                                   .setTiling(vk::ImageTiling::eOptimal)
                                   .setInitialLayout(vk::ImageLayout::eUndefined)
                                   .setSamples(vk::SampleCountFlagBits::e1),
                               "OffscreenRenderToImages")
                .setBaseFormat(colorFormat)
                .addViewInfo(vk::ImageViewCreateInfo()
                                 .setViewType(vk::ImageViewType::e2D)
                                 .setFormat(colorFormat)
                                 .setSubresourceRange(vk::ImageSubresourceRange()
                                                          .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                          .setBaseArrayLayer(0)
                                                          .setLayerCount(1)
                                                          .setBaseMipLevel(0)
                                                          .setLevelCount(1)));

        for (uint8_t i = 0; i < numFramesInFlight; i++)
        {
            colorTextures.emplace_back(builder.build());
        }
    }

    vk::Format depthFormat = vk::Format::eUndefined;
    std::vector<StarTextures::Texture> depthTextures;
    depthTextures.reserve(numFramesInFlight);
    {
        depthFormat =
            selectFormat(context, {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
                         vk::FormatFeatureFlagBits::eDepthStencilAttachment | vk::FormatFeatureFlagBits::eSampledImage);

        auto builder =
            star::StarTextures::Texture::Builder(context.getDevice().getVulkanDevice(),
                                                 context.getDevice().getAllocator().get())
                .setCreateInfo(
                    Allocator::AllocationBuilder()
                        .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                        .setUsage(VMA_MEMORY_USAGE_GPU_ONLY)
                        .build(),
                    vk::ImageCreateInfo()
                        .setExtent(vk::Extent3D().setWidth(width).setHeight(height).setDepth(1))
                        .setArrayLayers(1)
                        .setSharingMode(vk::SharingMode::eExclusive)
                        .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled)
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
                                                          .setLevelCount(1)))
                .setSamplerInfo(vk::SamplerCreateInfo()
                                    .setAnisotropyEnable(true)
                                    .setMaxAnisotropy(star::StarTextures::Texture::SelectAnisotropyLevel(props))
                                    .setMagFilter(star::StarTextures::Texture::SelectTextureFiltering(props))
                                    .setMinFilter(star::StarTextures::Texture::SelectTextureFiltering(props))
                                    .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
                                    .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
                                    .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
                                    .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
                                    .setUnnormalizedCoordinates(VK_FALSE)
                                    .setCompareEnable(VK_FALSE)
                                    .setCompareOp(vk::CompareOp::eAlways)
                                    .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                                    .setMipLodBias(0.0f)
                                    .setMinLod(0.0f)
                                    .setMaxLod(0.0f));

        for (uint8_t i = 0; i < numFramesInFlight; i++)
        {
            depthTextures.emplace_back(builder.build());
        }
    }

    auto colorHandles = registerTextures(context, renderingContext, std::move(colorTextures));
    auto depthHandles = registerTextures(context, renderingContext, std::move(depthTextures));
    return RenderTargets(std::move(colorHandles), colorFormat, std::move(depthHandles), depthFormat);
}
} // namespace star::core::renderer