#pragma once

#include "graphics/StarTextures/Texture.hpp"
#include "starlight/factory/TextureFactory.hpp"

namespace star::wrappers::graphics::policies
{
struct GenericImageCreateAllocatePolicy
{
    using Object = star::StarTextures::Texture;

    vk::Extent3D extent;
    vk::Format format;
    vk::ImageType imageType{vk::ImageType::e2D};
    vk::ImageUsageFlags usageFlags{vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc};
    std::string allocationName;
    VmaAllocationCreateInfo allocationCreateInfo;
    mutable VmaAllocator allocator;
    star::core::device::StarDevice *device = nullptr;

    Object create() const
    {
        auto createInfo = vk::ImageCreateInfo()
                              .setExtent(extent)
                              .setUsage(usageFlags)
                              .setImageType(imageType)
                              .setMipLevels(1)
                              .setArrayLayers(1)
                              .setTiling(vk::ImageTiling::eOptimal)
                              .setInitialLayout(vk::ImageLayout::eUndefined)
                              .setSamples(vk::SampleCountFlagBits::e1)
                              .setSharingMode(vk::SharingMode::eExclusive);

        const auto resources = star::StarTextures::TextureFactory::CreateResource(
            *device, format, allocator, allocationCreateInfo, createInfo, allocationName);
        const uint32_t mipmapLevels = createInfo.mipLevels;
        const vk::DeviceSize size = star::StarTextures::TextureFactory::CalculateSize(
            format, createInfo.extent, createInfo.arrayLayers, createInfo.imageType, mipmapLevels);

        return Object(resources, format, mipmapLevels, createInfo.extent, size);
    }
};
} // namespace star::wrappers::graphics::policies