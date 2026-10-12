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

namespace star::StarTextures
{

void TextureFactory::CreateAllocation(core::device::StarDevice &device, const vk::Format &baseFormat,
                                      VmaAllocator &allocator, const VmaAllocationCreateInfo &allocationCreateInfo,
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

TextureView TextureFactory::CreateViews(core::device::StarDevice &device, vk::Image image,
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

void TextureFactory::VerifyImageCreateInfo(const vk::ImageCreateInfo &createInfo)
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

void TextureFactory::LogImageCreateFailure(const vk::Result &result)
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

std::shared_ptr<Resources> TextureFactory::CreateImage(core::device::StarDevice &device,
                                                       const Texture::Creators &creators, const vk::Format &baseFormat)
{
    VerifyNewAllocation(device, creators, baseFormat);

    return CreateResource(device, baseFormat, device.getAllocator().get(), creators.allocationCreateInfo,
                          creators.createInfo, creators.allocationName);
}

std::shared_ptr<Resources> TextureFactory::CreateResource(core::device::StarDevice &device,
                                                          const vk::Format &baseFormat, VmaAllocator &allocator,
                                                          const VmaAllocationCreateInfo &allocationCreateInfo,
                                                          const vk::ImageCreateInfo &imageCreateInfo,
                                                          const std::string &allocationName)
{
    VerifyImageCreateInfo(imageCreateInfo);

    vk::Image image;
    VmaAllocation allocation;

    CreateAllocation(device, baseFormat, allocator, allocationCreateInfo, imageCreateInfo, allocation, image,
                     allocationName);

    return std::make_shared<AllocatedResources>(image, allocation, allocator);
}

void TextureFactory::VerifyNewAllocation(core::device::StarDevice &device, const Texture::Creators &creators,
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

Texture TextureFactory::FinishNewAllocation(const Texture::Creators &creators, const vk::Format &baseFormat,
                                            uint32_t mipmapLevels, std::shared_ptr<Resources> resources)
{
    const vk::DeviceSize size = CalculateSize(baseFormat, creators.createInfo.extent, creators.createInfo.arrayLayers,
                                              creators.createInfo.imageType, mipmapLevels);

    return Texture(std::move(resources), baseFormat, mipmapLevels, creators.createInfo.extent, size);
}

Texture TextureFactory::CreateTexture(core::device::StarDevice &device, const vk::Image &vulkanImage,
                                      const vk::Format &baseFormat, const TextureView &views, uint32_t mipmapLevels,
                                      const vk::Extent3D &resolution, vk::DeviceSize size)
{
    static_cast<void>(device);

    auto resources = std::make_shared<Resources>(vulkanImage, views);

    return Texture(std::move(resources), baseFormat, mipmapLevels, resolution, size);
}

Texture TextureFactory::CreateTexture(core::device::StarDevice &device, const vk::Image &vulkanImage,
                                      const vk::Format &baseFormat, const TextureView &views, uint32_t mipmapLevels,
                                      const vk::SamplerCreateInfo &samplerInfo, const vk::Extent3D &resolution,
                                      vk::DeviceSize size)
{
    const auto sampler = CreateImageSampler(device, samplerInfo);
    auto resources = std::make_shared<Resources>(vulkanImage, views, sampler);

    return Texture(std::move(resources), baseFormat, mipmapLevels, resolution, size);
}

vk::Sampler TextureFactory::CreateImageSampler(core::device::StarDevice &device,
                                               const vk::SamplerCreateInfo &samplerCreateInfo,
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

vk::DeviceSize TextureFactory::CalculateSize(const vk::Format &baseFormat, const vk::Extent3D &baseExtent,
                                             const uint32_t &arrayLayers, const vk::ImageType &imageType,
                                             const uint32_t &mipmapLevels)
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

uint32_t TextureFactory::CeilDiv(const uint32_t &width, const uint32_t &height)
{
    return (width + height - 1) / height;
}

vk::Extent3D TextureFactory::GetLevelExtent(const vk::Extent3D &baseExtent, const uint32_t &level)
{
    return vk::Extent3D()
        .setWidth(std::max(1u, baseExtent.width >> level))
        .setHeight(std::max(1u, baseExtent.height >> level))
        .setDepth(std::max(1u, baseExtent.depth >> level));
}
} // namespace star::StarTextures
