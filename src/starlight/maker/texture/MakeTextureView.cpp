#include "starlight/maker/texture/MakeTextureView.hpp"

#include "starlight/core/device/StarDevice.hpp"
#include "starlight/factory/TextureFactory.hpp"

#include <new>
#include <stdexcept>
#include <utility>

namespace star::maker::texture
{
static std::unexpected<TextureViewMakeError> Err(TextureViewMakeError::Cause cause, std::string detail,
                                                 size_t built = 0)
{
    return std::unexpected(TextureViewMakeError{cause, std::move(detail), built});
}

MakeTextureView::MakeTextureView(core::device::StarDevice &device) : m_device(device)
{
}

MakeTextureView &MakeTextureView::addViewInfo(const vk::ImageViewCreateInfo &info)
{
    m_views.push_back(info);
    return *this;
}

MakeTextureView &MakeTextureView::setImage(vk::Image image)
{
    m_image = image;
    return *this;
}

MakeTextureView &MakeTextureView::setMipmapLevels(uint32_t mipmapLevels)
{
    m_mipmapLevels = mipmapLevels;
    return *this;
}

MakeTextureView &MakeTextureView::setDebugName(std::string debugName)
{
    m_debugName = std::move(debugName);
    return *this;
}

std::expected<size_t, TextureViewMakeError> MakeTextureView::make(star::StarTextures::TextureView *dst,
                                                                  size_t num) noexcept
{
    if (m_image == VK_NULL_HANDLE)
    {
        return Err(TextureViewMakeError::Cause::invalid_config, "Image must be set before building a texture view");
    }

    size_t created = 0;
    try
    {
        for (; created < num; ++created)
        {
            new (dst + created) star::StarTextures::TextureView(createView());
        }
    }
    catch (const vk::OutOfHostMemoryError &e)
    {
        return Err(TextureViewMakeError::Cause::out_of_memory, e.what(), created);
    }
    catch (const vk::OutOfDeviceMemoryError &e)
    {
        return Err(TextureViewMakeError::Cause::out_of_memory, e.what(), created);
    }
    catch (const vk::OutOfPoolMemoryError &e)
    {
        return Err(TextureViewMakeError::Cause::out_of_memory, e.what(), created);
    }
    catch (const vk::MemoryMapFailedError &e)
    {
        return Err(TextureViewMakeError::Cause::out_of_memory, e.what(), created);
    }
    catch (const vk::DeviceLostError &e)
    {
        return Err(TextureViewMakeError::Cause::device_lost, e.what(), created);
    }
    catch (const std::bad_alloc &)
    {
        return Err(TextureViewMakeError::Cause::out_of_memory, "Failed to allocate memory for texture view", created);
    }
    catch (const std::exception &e)
    {
        return Err(TextureViewMakeError::Cause::invalid_config, e.what(), created);
    }
    catch (...)
    {
        return Err(TextureViewMakeError::Cause::invalid_config, "Unknown error while creating texture view", created);
    }

    return created;
}

star::StarTextures::TextureView MakeTextureView::createView()
{
    return star::StarTextures::TextureFactory::CreateViews(m_device, m_image, m_views, m_mipmapLevels, m_debugName);
}
} // namespace star::maker::texture
