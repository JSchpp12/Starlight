#include "starlight/maker/texture/MakeTexture.hpp"

#include "starlight/factory/TextureFactory.hpp"

#include <cassert>
#include <new>
#include <stdexcept>

namespace star::maker::texture
{
static std::unexpected<TextureMakeError> Err(TextureMakeError::Cause cause, std::string detail, size_t built = 0)
{
    return std::unexpected(TextureMakeError{cause, std::move(detail), built});
}

MakeTexture::MakeTexture(core::device::StarDevice &device, const vk::Image &vulkanImage)
    : m_device(device), m_vulkanImage(vulkanImage), m_viewInfos(device)
{
}

MakeTexture::MakeTexture(core::device::StarDevice &device) : m_device(device), m_viewInfos(device)
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
    m_viewInfos.addViewInfo(nCreateInfo);
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

std::expected<size_t, TextureMakeError> MakeTexture::make(StarTextures::Texture *dst, size_t num) noexcept
{
    if (!m_format.has_value())
    {
        return Err(TextureMakeError::Cause::invalid_config, "Base format must be set before building a texture");
    }

    if (m_createNewAllocationInfo.has_value() && m_createNewAllocationInfo->createInfo.mipLevels != m_mipmapLevels)
    {
        return Err(TextureMakeError::Cause::mip_mismatch,
                   "Image create info mip levels do not match the requested number of mipmap levels");
    }

    size_t created = 0;
    try
    {
        for (; created < num; ++created)
        {
            new (dst + created) StarTextures::Texture(createTexture());
        }
    }
    catch (const vk::OutOfHostMemoryError &e)
    {
        return Err(TextureMakeError::Cause::out_of_memory, e.what(), created);
    }
    catch (const vk::OutOfDeviceMemoryError &e)
    {
        return Err(TextureMakeError::Cause::out_of_memory, e.what(), created);
    }
    catch (const vk::OutOfPoolMemoryError &e)
    {
        return Err(TextureMakeError::Cause::out_of_memory, e.what(), created);
    }
    catch (const vk::MemoryMapFailedError &e)
    {
        return Err(TextureMakeError::Cause::out_of_memory, e.what(), created);
    }
    catch (const vk::DeviceLostError &e)
    {
        return Err(TextureMakeError::Cause::device_lost, e.what(), created);
    }
    catch (const vk::FormatNotSupportedError &e)
    {
        return Err(TextureMakeError::Cause::unsupported_format, e.what(), created);
    }
    catch (const vk::ImageUsageNotSupportedKHRError &e)
    {
        return Err(TextureMakeError::Cause::unsupported_format, e.what(), created);
    }
    catch (const std::bad_alloc &)
    {
        return Err(TextureMakeError::Cause::out_of_memory, "Failed to allocate memory for texture", created);
    }
    catch (const std::exception &e)
    {
        return Err(TextureMakeError::Cause::invalid_config, e.what(), created);
    }
    catch (...)
    {
        return Err(TextureMakeError::Cause::invalid_config, "Unknown error while creating texture", created);
    }

    return created;
}

StarTextures::Texture MakeTexture::createTexture()
{
    assert(m_format.has_value());
    const vk::Format &format = m_format.value();

    if (m_createNewAllocationInfo.has_value())
    {
        const StarTextures::Texture::Creators &creators = m_createNewAllocationInfo.value();

        auto resources = StarTextures::TextureFactory::CreateImage(m_device, creators, format);
        buildViewsAndSampler(*resources, creators.allocationName);

        return StarTextures::TextureFactory::FinishNewAllocation(creators, format, m_mipmapLevels,
                                                                 std::move(resources));
    }

    if (m_vulkanImage.has_value())
    {
        assert(m_overrideSize.has_value() && "Size MUST be calculated/provided for externally created/managed texture");
        assert(m_overrideResolution.has_value() &&
               "Resolution MUST be provided for externally created/managed texture");

        const auto views = buildViews(m_vulkanImage.value(), {});

        if (m_samplerInfo.has_value())
        {
            return StarTextures::TextureFactory::CreateTexture(m_device, m_vulkanImage.value(), format, views,
                                                               m_mipmapLevels, m_samplerInfo.value(),
                                                               m_overrideResolution.value(), m_overrideSize.value());
        }

        return StarTextures::TextureFactory::CreateTexture(m_device, m_vulkanImage.value(), format, views,
                                                           m_mipmapLevels, m_overrideResolution.value(),
                                                           m_overrideSize.value());
    }

    throw std::runtime_error("Invalid builder config");
}

StarTextures::TextureView MakeTexture::buildViews(vk::Image image, const std::string &debugName)
{
    m_viewInfos.setImage(image).setMipmapLevels(m_mipmapLevels).setDebugName(debugName);

    star::StarTextures::TextureView views;
    const auto result = m_viewInfos.make(&views, 1);
    if (!result)
    {
        throw std::runtime_error("Failed to build texture views: " + result.error().detail);
    }

    return views;
}

void MakeTexture::buildViewsAndSampler(StarTextures::Resources &resources, const std::string &debugName)
{
    resources.views = buildViews(resources.image, debugName);

    if (m_samplerInfo.has_value())
    {
        resources.sampler = StarTextures::TextureFactory::CreateImageSampler(m_device.getVulkanDevice(),
                                                                             m_samplerInfo.value(), debugName);
    }
}
} // namespace star::maker::texture
