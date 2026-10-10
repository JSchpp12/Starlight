#include "starlight/command/texture/MakeTexture.hpp"

#include "starlight/factory/TextureFactory.hpp"

#include <cassert>
#include <stdexcept>

namespace star::command::texture
{
MakeTexture::MakeTexture(core::device::StarDevice &device, const vk::Image &vulkanImage)
    : m_device(device), m_vulkanImage(vulkanImage)
{
}

MakeTexture::MakeTexture(core::device::StarDevice &device) : m_device(device)
{
}

MakeTexture &MakeTexture::setCreateInfo(VmaAllocationCreateInfo nAllocInfo, vk::ImageCreateInfo nCreateInfo,
                                        std::string nAllocationName)
{
    m_createNewAllocationInfo = std::make_optional<StarTextures::Texture::Creators>(
        StarTextures::Texture::Creators(std::move(nCreateInfo), std::move(nAllocInfo), std::move(nAllocationName)));
    return *this;
}

MakeTexture &MakeTexture::addViewInfo(const vk::ImageViewCreateInfo &nCreateInfo)
{
    m_viewInfos.add(nCreateInfo);
    return *this;
}

MakeTexture &MakeTexture::setSamplerInfo(const vk::SamplerCreateInfo &nCreateInfo)
{
    m_samplerInfo = nCreateInfo;
    return *this;
}

MakeTexture &MakeTexture::setBaseFormat(const vk::Format &nFormat)
{
    m_format = nFormat;
    return *this;
}

MakeTexture &MakeTexture::setMipmapLevels(uint32_t nLevels)
{
    m_mipmapLevels = nLevels;
    return *this;
}

MakeTexture &MakeTexture::setSizeInfo(vk::DeviceSize size, vk::Extent3D resolution)
{
    m_overrideSize = std::make_optional(size);
    m_overrideResolution = std::make_optional(resolution);

    return *this;
}

std::unique_ptr<StarTextures::Texture> MakeTexture::buildUnique()
{
    return std::make_unique<StarTextures::Texture>(build());
}

std::shared_ptr<StarTextures::Texture> MakeTexture::buildShared()
{
    return std::make_shared<StarTextures::Texture>(build());
}

StarTextures::Texture MakeTexture::build()
{
    assert(m_format.has_value());
    const vk::Format &format = m_format.value();

    if (m_createNewAllocationInfo.has_value())
    {
        const StarTextures::Texture::Creators &creators = m_createNewAllocationInfo.value();

        if (m_samplerInfo.has_value())
        {
            return StarTextures::TextureFactory::CreateTexture(m_device, creators, format, m_viewInfos, m_mipmapLevels,
                                                               m_samplerInfo.value());
        }

        return StarTextures::TextureFactory::CreateTexture(m_device, creators, format, m_viewInfos, m_mipmapLevels);
    }

    if (m_vulkanImage.has_value())
    {
        assert(m_overrideSize.has_value() && "Size MUST be calculated/provided for externally created/managed texture");
        assert(m_overrideResolution.has_value() &&
               "Resolution MUST be provided for externally created/managed texture");

        if (m_samplerInfo.has_value())
        {
            return StarTextures::TextureFactory::CreateTexture(m_device, m_vulkanImage.value(), format, m_viewInfos,
                                                               m_mipmapLevels, m_samplerInfo.value(),
                                                               m_overrideResolution.value(), m_overrideSize.value());
        }

        return StarTextures::TextureFactory::CreateTexture(m_device, m_vulkanImage.value(), format, m_viewInfos,
                                                           m_mipmapLevels, m_overrideResolution.value(),
                                                           m_overrideSize.value());
    }

    throw std::runtime_error("Invalid builder config");
}
} // namespace star::command::texture
