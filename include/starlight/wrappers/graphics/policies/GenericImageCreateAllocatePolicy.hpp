#pragma once

#include "graphics/StarTextures/Texture.hpp"

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
    mutable vk::Device device;

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

        return Object(device, allocator, createInfo, allocationName, allocationCreateInfo, {}, format, {});
    }
};
} // namespace star::wrappers::graphics::policies