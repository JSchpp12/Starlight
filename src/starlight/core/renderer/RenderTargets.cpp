#include "core/renderer/RenderTargets.hpp"

#include "Allocator.hpp"
#include "ManagerRenderResource.hpp"
#include "StarTextures/Texture.hpp"
#include "core/device/DeviceContext.hpp"
#include "core/device/system/event/ManagerRequest.hpp"

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

    if (!m_colorHandles.empty())
    {
        renderingContext.recordDependentImage.manualInsert(m_colorHandles[fi],
                                                           &context.getImageManager().get(m_colorHandles[fi])->texture);
    }
    if (!m_depthHandles.empty())
    {
        renderingContext.recordDependentImage.manualInsert(m_depthHandles[fi],
                                                           &context.getImageManager().get(m_depthHandles[fi])->texture);
    }
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

std::vector<StarTextures::Texture> RenderTargets::createDefaultColorAttachments(core::device::DeviceContext &context,
                                                                                const size_t numToCreate, int width,
                                                                                int height)
{
    vk::Format colorFormat = vk::Format::eUndefined;
    std::vector<StarTextures::Texture> colorTextures;
    colorTextures.reserve(numToCreate);
    {
        colorFormat =
            selectFormat(context, {vk::Format::eR8G8B8A8Srgb, vk::Format::eR8G8B8A8Unorm},
                         vk::FormatFeatureFlagBits::eColorAttachment | vk::FormatFeatureFlagBits::eTransferSrc |
                             vk::FormatFeatureFlagBits::eStorageImage);

        auto builder =
            star::StarTextures::Texture::Builder(context.getDevice())
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

        for (uint8_t i = 0; i < numToCreate; i++)
        {
            colorTextures.emplace_back(builder.build());
        }
    }

    return colorTextures;
}

std::vector<StarTextures::Texture> RenderTargets::createDefaultDepthAttachments(core::device::DeviceContext &context,
                                                                                const size_t numToCreate, int width,
                                                                                int height)
{
    const auto &props = context.getDevice().getPhysicalDevice().getProperties();

    vk::Format depthFormat = vk::Format::eUndefined;
    std::vector<StarTextures::Texture> depthTextures;
    depthTextures.reserve(numToCreate);
    {
        depthFormat =
            selectFormat(context, {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
                         vk::FormatFeatureFlagBits::eDepthStencilAttachment | vk::FormatFeatureFlagBits::eSampledImage);

        auto builder =
            star::StarTextures::Texture::Builder(context.getDevice())
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

        for (uint8_t i = 0; i < numToCreate; i++)
        {
            depthTextures.push_back(builder.build());
        }
    }

    return depthTextures;
}

RenderTargets RenderTargets::forOffscreen(core::device::DeviceContext &context, RenderingContext &renderingContext)
{
    const uint8_t numFramesInFlight = static_cast<uint8_t>(context.frameTracker().getSetup().getNumFramesInFlight());

    int width, height;
    engineResolution(context, width, height);
    const auto &props = context.getDevice().getPhysicalDevice().getProperties();

    auto colorTextures = createDefaultColorAttachments(context, numFramesInFlight, width, height);
    if (colorTextures.size() == 0)
        STAR_THROW("Failed to create default color attachments");

    const auto colorFormat = colorTextures.front().getBaseFormat();
    auto depthTextures = createDefaultDepthAttachments(context, numFramesInFlight, width, height);
    if (depthTextures.size() == 0)
        STAR_THROW("Failed to create default depth attachments");

    auto depthFormat = depthTextures.front().getBaseFormat();
    auto colorHandles = registerTextures(context, renderingContext, std::move(colorTextures));
    auto depthHandles = registerTextures(context, renderingContext, std::move(depthTextures));
    return RenderTargets(std::move(colorHandles), colorFormat, std::move(depthHandles), depthFormat);
}
} // namespace star::core::renderer
