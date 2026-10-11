#include "starlight/factory/TextureFactory.hpp"

#include "StarTextures/AllocatedResources.hpp"
#include "StarTextures/FormatInfo.hpp"
#include "logging/LoggingFactory.hpp"
#include "starlight/core/Exceptions.hpp"

#include <algorithm>
#include <cassert>
#include <memory>
#include <sstream>

namespace star::StarTextures
{
void TextureFactory::CreateAllocation(vk::Device &device, const vk::Format &baseFormat, VmaAllocator &allocator,
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
    {
        vk::DebugUtilsObjectNameInfoEXT nameInfo = vk::DebugUtilsObjectNameInfoEXT()
                                                       .setObjectType(vk::ObjectType::eImage)
                                                       .setObjectHandle((uint64_t)(VkImage)(textureImage))
                                                       .setPObjectName(allocationName.c_str());

        device.setDebugUtilsObjectNameEXT(nameInfo);

        const std::string fullAllocationName = std::string(allocationName) + "_ALLOCATION";
        vmaSetAllocationName(allocator, allocation, fullAllocationName.c_str());
    }
#endif
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

std::shared_ptr<Resources> TextureFactory::CreateResource(vk::Device &device, const vk::Format &baseFormat,
                                                          VmaAllocator &allocator,
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

std::shared_ptr<Resources> TextureFactory::CreateResource(vk::Device &device, const vk::Format &baseFormat,
                                                          VmaAllocator &allocator,
                                                          const VmaAllocationCreateInfo &allocationCreateInfo,
                                                          const vk::ImageCreateInfo &imageCreateInfo,
                                                          const std::string &allocationName,
                                                          const TextureViewBuilder &viewInfos, uint32_t mipmapLevels)
{
    VerifyImageCreateInfo(imageCreateInfo);

    vk::Image image{VK_NULL_HANDLE};
    VmaAllocation allocation;

    CreateAllocation(device, baseFormat, allocator, allocationCreateInfo, imageCreateInfo, allocation, image,
                     allocationName);

    const auto views = viewInfos.build(device, image, mipmapLevels);

    return std::make_shared<AllocatedResources>(image, views, allocation, allocator);
}

std::shared_ptr<Resources> TextureFactory::CreateResource(vk::Device &device, const vk::Format &baseFormat,
                                                          VmaAllocator &allocator,
                                                          const VmaAllocationCreateInfo &allocationCreateInfo,
                                                          const vk::ImageCreateInfo &imageCreateInfo,
                                                          const std::string &allocationName,
                                                          const TextureViewBuilder &viewInfos, uint32_t mipmapLevels,
                                                          const vk::SamplerCreateInfo &samplerCreateInfo)
{
    VerifyImageCreateInfo(imageCreateInfo);

    vk::Image image{VK_NULL_HANDLE};
    VmaAllocation allocation;

    CreateAllocation(device, baseFormat, allocator, allocationCreateInfo, imageCreateInfo, allocation, image,
                     allocationName);

    const auto views = viewInfos.build(device, image, mipmapLevels);

    const auto sampler = CreateImageSampler(device, samplerCreateInfo);

    return std::make_shared<AllocatedResources>(image, views, sampler, allocation, allocator);
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

Texture TextureFactory::CreateTexture(core::device::StarDevice &device, const Texture::Creators &creators,
                                      const vk::Format &baseFormat, const TextureViewBuilder &viewInfos,
                                      uint32_t mipmapLevels)
{
    VerifyNewAllocation(device, creators, baseFormat);

    auto &allocator = device.getAllocator().get();
    auto resources = CreateResource(device.getVulkanDevice(), baseFormat, allocator, creators.allocationCreateInfo,
                                    creators.createInfo, creators.allocationName, viewInfos, mipmapLevels);

    return FinishNewAllocation(creators, baseFormat, mipmapLevels, std::move(resources));
}

Texture TextureFactory::CreateTexture(core::device::StarDevice &device, const Texture::Creators &creators,
                                      const vk::Format &baseFormat, const TextureViewBuilder &viewInfos,
                                      uint32_t mipmapLevels, const vk::SamplerCreateInfo &samplerInfo)
{
    VerifyNewAllocation(device, creators, baseFormat);

    auto &allocator = device.getAllocator().get();
    auto resources = CreateResource(device.getVulkanDevice(), baseFormat, allocator, creators.allocationCreateInfo,
                                    creators.createInfo, creators.allocationName, viewInfos, mipmapLevels, samplerInfo);

    return FinishNewAllocation(creators, baseFormat, mipmapLevels, std::move(resources));
}

Texture TextureFactory::CreateTexture(core::device::StarDevice &device, const vk::Image &vulkanImage,
                                      const vk::Format &baseFormat, const TextureViewBuilder &viewInfos,
                                      uint32_t mipmapLevels, const vk::Extent3D &resolution, vk::DeviceSize size)
{
    const auto views = viewInfos.build(device.getVulkanDevice(), vulkanImage, mipmapLevels);
    auto resources = std::make_shared<Resources>(vulkanImage, views);

    return Texture(std::move(resources), baseFormat, mipmapLevels, resolution, size);
}

Texture TextureFactory::CreateTexture(core::device::StarDevice &device, const vk::Image &vulkanImage,
                                      const vk::Format &baseFormat, const TextureViewBuilder &viewInfos,
                                      uint32_t mipmapLevels, const vk::SamplerCreateInfo &samplerInfo,
                                      const vk::Extent3D &resolution, vk::DeviceSize size)
{
    const auto views = viewInfos.build(device.getVulkanDevice(), vulkanImage, mipmapLevels);
    const auto sampler = CreateImageSampler(device.getVulkanDevice(), samplerInfo);
    auto resources = std::make_shared<Resources>(vulkanImage, views, sampler);

    return Texture(std::move(resources), baseFormat, mipmapLevels, resolution, size);
}

vk::Sampler TextureFactory::CreateImageSampler(vk::Device &device, const vk::SamplerCreateInfo &samplerCreateInfo)
{
    vk::Sampler sampler{device.createSampler(samplerCreateInfo)};

    if (!sampler)
    {
        STAR_THROW("Failed to create sampler");
    }

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
