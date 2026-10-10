#include "StarTextures/Texture.hpp"

#include "ConfigFile.hpp"
#include "starlight/command/texture/MakeTexture.hpp"
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

star::StarTextures::Texture::Builder::Builder(core::device::StarDevice &device, const vk::Image &vulkanImage)
    : m_makeTexture(std::make_unique<command::texture::MakeTexture>(device, vulkanImage))
{
}

star::StarTextures::Texture::Builder::Builder(core::device::StarDevice &device)
    : m_makeTexture(std::make_unique<command::texture::MakeTexture>(device))
{
}

star::StarTextures::Texture::Builder::Builder(Builder &&) noexcept = default;

star::StarTextures::Texture::Builder &star::StarTextures::Texture::Builder::operator=(Builder &&) noexcept = default;

star::StarTextures::Texture::Builder::Builder(const Builder &other)
    : m_makeTexture(other.m_makeTexture ? std::make_unique<command::texture::MakeTexture>(*other.m_makeTexture)
                                        : nullptr)
{
}

star::StarTextures::Texture::Builder &star::StarTextures::Texture::Builder::operator=(const Builder &other)
{
    if (this != &other)
    {
        m_makeTexture =
            other.m_makeTexture ? std::make_unique<command::texture::MakeTexture>(*other.m_makeTexture) : nullptr;
    }
    return *this;
}

star::StarTextures::Texture::Builder::~Builder() = default;

star::StarTextures::Texture::Builder &star::StarTextures::Texture::Builder::setCreateInfo(
    VmaAllocationCreateInfo nAllocInfo, vk::ImageCreateInfo nCreateInfo, std::string nAllocName)
{
    m_makeTexture->setCreateInfo(nAllocInfo, std::move(nCreateInfo), std::move(nAllocName));
    return *this;
}

star::StarTextures::Texture::Builder &star::StarTextures::Texture::Builder::addViewInfo(
    const vk::ImageViewCreateInfo &nCreateInfo)
{
    m_makeTexture->addViewInfo(nCreateInfo);
    return *this;
}

star::StarTextures::Texture::Builder &star::StarTextures::Texture::Builder::setSamplerInfo(
    const vk::SamplerCreateInfo &nCreateInfo)
{
    m_makeTexture->setSamplerInfo(nCreateInfo);
    return *this;
}

star::StarTextures::Texture::Builder &star::StarTextures::Texture::Builder::setBaseFormat(const vk::Format &nFormat)
{
    m_makeTexture->setBaseFormat(nFormat);
    return *this;
}

star::StarTextures::Texture::Builder &star::StarTextures::Texture::Builder::setMipmapLevels(uint32_t nLevels)
{
    m_makeTexture->setMipmapLevels(nLevels);
    return *this;
}

star::StarTextures::Texture::Builder &star::StarTextures::Texture::Builder::setSizeInfo(vk::DeviceSize size,
                                                                                        vk::Extent3D resolution)
{
    m_makeTexture->setSizeInfo(size, resolution);
    return *this;
}

std::unique_ptr<star::StarTextures::Texture> star::StarTextures::Texture::Builder::buildUnique()
{
    return m_makeTexture->buildUnique();
}

std::shared_ptr<star::StarTextures::Texture> star::StarTextures::Texture::Builder::buildShared()
{
    return m_makeTexture->buildShared();
}

star::StarTextures::Texture star::StarTextures::Texture::Builder::build()
{
    return m_makeTexture->build();
}