#include "starlight/factory/TextureFactory.hpp"

#include "StarTextures/AllocatedResources.hpp"
#include "StarTextures/FormatInfo.hpp"
#include "logging/LoggingFactory.hpp"
#include "starlight/core/Exceptions.hpp"

#include <algorithm>
#include <cassert>
#include <memory>
#include <sstream>
#include <string_view>

namespace star::StarTextures::TextureFactory
{
static void LogImageCreateFailure(const vk::Result &result)
{
    std::ostringstream oss;
    oss << "Failed to create image with Vulkan error: ";

    if (result == vk::Result::eErrorFeatureNotPresent)
    {
        oss << "eErrorFeatureNotPresent";
    }
    else
    {
        oss << "Undefined error";
    }

    core::logging::log(boost::log::trivial::error, oss.str());
}

static void LogTextureAllocation(const std::string &name, const vk::Format &format,
                                 const vk::ImageCreateInfo &createInfo, vk::DeviceSize size)
{
    core::logging::debug("Allocated texture \"", name, "\": ", createInfo.extent.width, "x", createInfo.extent.height,
                         "x", createInfo.extent.depth, ", size=", size, " bytes, format=", vk::to_string(format),
                         ", usage=", vk::to_string(createInfo.usage), ", mipLevels=", createInfo.mipLevels,
                         ", arrayLayers=", createInfo.arrayLayers);
}

static void LogExternalTextureWrap(const std::string &name, const vk::Format &format, const vk::Extent3D &extent,
                                   uint32_t mipLevels, vk::DeviceSize size)
{
    core::logging::debug("Wrapped external texture \"", name, "\": ", extent.width, "x", extent.height, "x",
                         extent.depth, ", size=", size, " bytes, format=", vk::to_string(format),
                         ", usage=externally managed, mipLevels=", mipLevels);
}

static void VerifyImageCreateInfo(const vk::ImageCreateInfo &createInfo)
{
    assert(createInfo.extent.width != 0 && "width cannot be 0");
    assert(createInfo.extent.height != 0 && "height cannot be 0");
    assert(createInfo.extent.depth != 0 && "depth cannot be 0");
    assert(createInfo.mipLevels != 0 && "mip levels cannnot be 0");
    assert(createInfo.arrayLayers != 0 && "array layers cannot be 0");
    assert((createInfo.initialLayout == vk::ImageLayout::eUndefined) ||
           createInfo.initialLayout == vk::ImageLayout::ePreinitialized &&
               "Invalid initial layout according to vulkan specs");
    assert(createInfo.sharingMode == vk::SharingMode::eExclusive ||
           (createInfo.sharingMode == vk::SharingMode::eConcurrent && createInfo.queueFamilyIndexCount > 1) &&
               "Must have indices defined for shared resource");
}

static void CreateAllocation(core::device::StarDevice &device, const vk::Format &baseFormat, VmaAllocator &allocator,
                             const VmaAllocationCreateInfo &allocationCreateInfo,
                             const vk::ImageCreateInfo &imageCreateInfo, VmaAllocation &allocation,
                             vk::Image &textureImage, const std::string &allocationName)
{
    vk::ImageCreateInfo addFormat = vk::ImageCreateInfo(imageCreateInfo).setFormat(baseFormat);

    vk::Result result;
    {
        auto tmpResult = vmaCreateImage(allocator, (VkImageCreateInfo *)&addFormat, &allocationCreateInfo,
                                        (VkImage *)&textureImage, &allocation, nullptr);
        result = static_cast<vk::Result>(tmpResult);
    }

    if (result != vk::Result::eSuccess)
    {
        LogImageCreateFailure(result);

        vk::detail::resultCheck(result, "Failed to create image via vmaCreateImage");
    }

#ifndef NDEBUG
    device.assignDebugName(vk::ObjectType::eImage, (uint64_t)(VkImage)(textureImage), allocationName);
    const std::string fullAllocationName = std::string(allocationName) + "_ALLOCATION";
    vmaSetAllocationName(allocator, allocation, fullAllocationName.c_str());
#endif
}

static void VerifyNewAllocation(core::device::StarDevice &device, const Texture::Creators &creators,
                                const vk::Format &baseFormat)
{
    // Build the effective create info with the baseFormat that CreateAllocation will actually use
    const vk::ImageCreateInfo effectiveInfo = vk::ImageCreateInfo(creators.createInfo).setFormat(baseFormat);

    if (!device.verifyImageCreate(effectiveInfo))
    {
        std::ostringstream oss;
        oss << "Hardware does not support creating image with format " << vk::to_string(baseFormat) << ", usage "
            << vk::to_string(effectiveInfo.usage) << ", tiling " << vk::to_string(effectiveInfo.tiling);
        throw vk::FormatNotSupportedError(oss.str());
    }
}

inline static uint32_t CeilDiv(const uint32_t &width, const uint32_t &height)
{
    return (width + height - 1) / height;
}

inline static vk::Extent3D GetLevelExtent(const vk::Extent3D &baseExtent, const uint32_t &level)
{
    return vk::Extent3D()
        .setWidth(std::max(1u, baseExtent.width >> level))
        .setHeight(std::max(1u, baseExtent.height >> level))
        .setDepth(std::max(1u, baseExtent.depth >> level));
}

TextureView CreateViews(core::device::StarDevice &device, vk::Image image,
                        std::span<const vk::ImageViewCreateInfo> viewInfos, uint32_t mipmapLevels,
                        const std::string &debugName)
{
    TextureView textureView;

#ifndef NDEBUG
    size_t viewIndex = 0;
#endif

    for (const vk::ImageViewCreateInfo &viewInfo : viewInfos)
    {
        if (viewInfo.subresourceRange.levelCount != mipmapLevels)
        {
            std::ostringstream oss;
            oss << "ImageViewCreateInfo levelCount (" << viewInfo.subresourceRange.levelCount
                << ") does not match the texture mipmapLevels (" << mipmapLevels << ")";
            STAR_THROW(oss.str());
        }

        assert(!textureView.contains(viewInfo.format) &&
               "Duplicate format requested for view. Only one view per format is supported");

        vk::ImageViewCreateInfo info = viewInfo;
        info.image = image;

        vk::ImageView view = device.getVulkanDevice().createImageView(info);

#ifndef NDEBUG
        if (!debugName.empty())
        {
            device.assignDebugName(vk::ObjectType::eImageView, (uint64_t)(VkImageView)(view),
                                   debugName + "_VIEW_" + std::to_string(viewIndex));
        }
        ++viewIndex;
#endif

        textureView.addView(info.format, view);
    }

    return textureView;
}

std::shared_ptr<Resources> CreateImage(core::device::StarDevice &device, const Texture::Creators &creators,
                                       const vk::Format &baseFormat)
{
    VerifyNewAllocation(device, creators, baseFormat);

    return CreateResource(device, baseFormat, device.getAllocator().get(), creators.allocationCreateInfo,
                          creators.createInfo, creators.allocationName);
}

std::shared_ptr<Resources> CreateResource(core::device::StarDevice &device, const vk::Format &baseFormat,
                                          VmaAllocator &allocator, const VmaAllocationCreateInfo &allocationCreateInfo,
                                          const vk::ImageCreateInfo &imageCreateInfo, const std::string &allocationName)
{
    VerifyImageCreateInfo(imageCreateInfo);

    vk::Image image;
    VmaAllocation allocation;

    CreateAllocation(device, baseFormat, allocator, allocationCreateInfo, imageCreateInfo, allocation, image,
                     allocationName);

    const vk::DeviceSize size = CalculateSize(baseFormat, imageCreateInfo.extent, imageCreateInfo.arrayLayers,
                                              imageCreateInfo.imageType, imageCreateInfo.mipLevels);
    LogTextureAllocation(allocationName, baseFormat, imageCreateInfo, size);

    return std::make_shared<AllocatedResources>(image, allocation, allocator);
}

Texture FinishNewAllocation(const Texture::Creators &creators, const vk::Format &baseFormat, uint32_t mipmapLevels,
                            std::shared_ptr<Resources> resources)
{
    const vk::DeviceSize size = CalculateSize(baseFormat, creators.createInfo.extent, creators.createInfo.arrayLayers,
                                              creators.createInfo.imageType, mipmapLevels);

    return Texture(std::move(resources), baseFormat, mipmapLevels, creators.createInfo.extent, size);
}

Texture CreateAllocatedTexture(core::device::StarDevice &device, const Texture::Creators &creators,
                               const vk::Format &baseFormat, std::span<const vk::ImageViewCreateInfo> viewInfos,
                               uint32_t mipmapLevels)
{
    std::shared_ptr<Resources> resources = CreateImage(device, creators, baseFormat);
    resources->views = CreateViews(device, resources->image, viewInfos, mipmapLevels, creators.allocationName);

    return FinishNewAllocation(creators, baseFormat, mipmapLevels, std::move(resources));
}

Texture CreateAllocatedTexture(core::device::StarDevice &device, const Texture::Creators &creators,
                               const vk::Format &baseFormat, std::span<const vk::ImageViewCreateInfo> viewInfos,
                               uint32_t mipmapLevels, const vk::SamplerCreateInfo &samplerInfo)
{
    std::shared_ptr<Resources> resources = CreateImage(device, creators, baseFormat);
    resources->views = CreateViews(device, resources->image, viewInfos, mipmapLevels, creators.allocationName);
    resources->sampler = CreateImageSampler(device, samplerInfo, creators.allocationName);

    return FinishNewAllocation(creators, baseFormat, mipmapLevels, std::move(resources));
}

Texture CreateTexture(core::device::StarDevice &device, const vk::Image &vulkanImage, const vk::Format &baseFormat,
                      std::span<const vk::ImageViewCreateInfo> viewInfos, uint32_t mipmapLevels,
                      const std::string &debugName, const vk::Extent3D &resolution, vk::DeviceSize size)
{
    device.assignDebugName(vk::ObjectType::eImage, (uint64_t)(VkImage)(vulkanImage), debugName);

    TextureView views = CreateViews(device, vulkanImage, viewInfos, mipmapLevels, debugName);
    auto resources = std::make_shared<Resources>(vulkanImage, views);

    LogExternalTextureWrap(debugName, baseFormat, resolution, mipmapLevels, size);

    return Texture(std::move(resources), baseFormat, mipmapLevels, resolution, size);
}

Texture CreateTexture(core::device::StarDevice &device, const vk::Image &vulkanImage, const vk::Format &baseFormat,
                      std::span<const vk::ImageViewCreateInfo> viewInfos, uint32_t mipmapLevels,
                      const vk::SamplerCreateInfo &samplerInfo, const std::string &debugName,
                      const vk::Extent3D &resolution, vk::DeviceSize size)
{
    device.assignDebugName(vk::ObjectType::eImage, (uint64_t)(VkImage)(vulkanImage), debugName);

    TextureView views = CreateViews(device, vulkanImage, viewInfos, mipmapLevels, debugName);
    vk::Sampler sampler = CreateImageSampler(device, samplerInfo, debugName);
    auto resources = std::make_shared<Resources>(vulkanImage, views, sampler);

    LogExternalTextureWrap(debugName, baseFormat, resolution, mipmapLevels, size);

    return Texture(std::move(resources), baseFormat, mipmapLevels, resolution, size);
}

vk::Sampler CreateImageSampler(core::device::StarDevice &device, const vk::SamplerCreateInfo &samplerCreateInfo,
                               const std::string &debugName)
{
    vk::Sampler sampler{device.getVulkanDevice().createSampler(samplerCreateInfo)};

#ifndef NDEBUG
    if (!debugName.empty())
    {
        device.assignDebugName(vk::ObjectType::eSampler, (uint64_t)(VkSampler)(sampler), debugName + "_SAMPLER");
    }
#endif

    return sampler;
}

vk::DeviceSize CalculateSize(const vk::Format &baseFormat, const vk::Extent3D &baseExtent, const uint32_t &arrayLayers,
                             const vk::ImageType &imageType, const uint32_t &mipmapLevels)
{
    const FormatInfo info{FormatInfo::Create(baseFormat)};
    vk::DeviceSize totalSize{0};

    for (uint32_t i = 0; i < mipmapLevels; i++)
    {
        auto levelExtent = GetLevelExtent(baseExtent, i);

        if (imageType == vk::ImageType::e1D)
        {
            levelExtent.height = 1;
            levelExtent.width = 1;
        }
        else if (imageType == vk::ImageType::e2D)
        {
            levelExtent.depth = 1;
        }

        vk::DeviceSize levelBytes = 0;

        if (info.blockCompressed)
        {
            const uint32_t bx = CeilDiv(levelExtent.width, info.blockExtent.width);
            const uint32_t by = CeilDiv(levelExtent.height, info.blockExtent.height);
            const uint32_t bz = CeilDiv(levelExtent.depth, info.blockExtent.depth);
            levelBytes = static_cast<vk::DeviceSize>(bx) * static_cast<vk::DeviceSize>(by) *
                         static_cast<vk::DeviceSize>(bz) * static_cast<vk::DeviceSize>(info.blockBytes);
        }
        else
        {
            levelBytes =
                static_cast<vk::DeviceSize>(levelExtent.width) * static_cast<vk::DeviceSize>(levelExtent.height) *
                static_cast<vk::DeviceSize>(levelExtent.depth) * static_cast<vk::DeviceSize>(info.bytesPerPixel);
        }

        totalSize += levelBytes;
    }

    return totalSize;
}
} // namespace star::StarTextures::TextureFactory
