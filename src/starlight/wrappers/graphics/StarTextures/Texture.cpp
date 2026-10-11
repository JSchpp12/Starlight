#include "StarTextures/Texture.hpp"

#include "ConfigFile.hpp"
#include "starlight/factory/TextureFactory.hpp"

#include <cassert>
#include <stdexcept>

void star::StarTextures::Texture::TransitionImageLayout(Texture &image, vk::CommandBuffer &commandBuffer,
                                                        const vk::Format &format, const vk::ImageLayout &oldLayout,
                                                        const vk::ImageLayout &newLayout)
{
    // create a barrier to prevent pipeline from moving forward until image transition is complete
    vk::ImageMemoryBarrier barrier{};
    barrier.sType = vk::StructureType::eImageMemoryBarrier; // specific flag for image operations
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;

    // if barrier is used for transferring ownership between queue families, this would be important -- set to ignore
    // since we are not doing this
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

    barrier.image = image.getVulkanImage();
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = image.getMipmapLevels();
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    // the operations that need to be completed before and after the barrier, need to be defined
    barrier.srcAccessMask = {}; // TODO
    barrier.dstAccessMask = {}; // TODO

    vk::PipelineStageFlags sourceStage, destinationStage;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
    {
        // undefined transition state, dont need to wait for this to complete
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
             (newLayout == vk::ImageLayout::eShaderReadOnlyOptimal || newLayout == vk::ImageLayout::eGeneral))
    {
        // transfer destination shader reading, will need to wait for completion. Especially in the frag shader where
        // reads will happen
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eNone;

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eBottomOfPipe;
    }
    else
    {
        throw std::invalid_argument("unsupported layout transition!");
    }

    // transfer writes must occurr during the pipeline transfer stage
    commandBuffer.pipelineBarrier(sourceStage,      // which pipeline stages should occurr before barrier
                                  destinationStage, // pipeline stage in which operations will wait on the barrier
                                  {}, {}, nullptr, barrier);
}

float star::StarTextures::Texture::SelectAnisotropyLevel(const vk::PhysicalDeviceProperties &deviceProperties)
{
    std::string anisotropySetting = star::ConfigFile::getSetting(star::Config_Settings::texture_anisotropy);
    float anisotropyLevel = 1.0f;

    if (anisotropySetting == "max")
        anisotropyLevel = deviceProperties.limits.maxSamplerAnisotropy;
    else
    {
        anisotropyLevel = std::stof(anisotropySetting);

        if (anisotropyLevel > deviceProperties.limits.maxSamplerAnisotropy)
        {
            anisotropyLevel = deviceProperties.limits.maxSamplerAnisotropy;
        }
        else if (anisotropyLevel < 1.0f)
        {
            anisotropyLevel = 1.0f;
        }
    }

    return anisotropyLevel;
}

vk::Filter star::StarTextures::Texture::SelectTextureFiltering(const vk::PhysicalDeviceProperties &deviceProperties)
{
    auto textureFilteringSetting = ConfigFile::getSetting(Config_Settings::texture_filtering);

    vk::Filter filterType;
    if (textureFilteringSetting == "nearest")
    {
        filterType = vk::Filter::eNearest;
    }
    else if (textureFilteringSetting == "linear")
    {
        filterType = vk::Filter::eLinear;
    }
    else
    {
        throw std::runtime_error("Texture filtering setting must be 'nearest' or 'linear'");
    }

    return filterType;
}

vk::ImageView star::StarTextures::Texture::getImageView(const vk::Format *requestedFormat) const
{
    // Forward the lookup into the resource's view storage. When no format is requested fall back to the texture's base
    // format instead of an arbitrary view.
    const vk::Format format = requestedFormat != nullptr ? *requestedFormat : baseFormat;

    assert(memoryResources->views.contains(format) &&
           "The image must be created with the proper image view before being requested");

    return memoryResources->views.get(format);
}

star::StarTextures::Texture::Texture(std::shared_ptr<Resources> memoryResources, const vk::Format &baseFormat,
                                     uint32_t mipmapLevels, const vk::Extent3D &baseExtent, vk::DeviceSize size)
    : memoryResources(std::move(memoryResources)), baseFormat(baseFormat), mipmapLevels(mipmapLevels),
      baseExtent(baseExtent), size(size)
{
}
