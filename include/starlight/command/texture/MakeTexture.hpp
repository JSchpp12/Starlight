#pragma once

#include "starlight/wrappers/graphics/StarTextures/Texture.hpp"
#include "starlight/wrappers/graphics/StarTextures/TextureViewBuilder.hpp"

#include <star_common/IServiceCommandWithReply.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace star::command::texture
{
namespace make_texture
{
inline constexpr const char *GetUniqueTypeName()
{
    return "scMakeTexture";
}
} // namespace make_texture

/// @brief Request that a texture be constructed from either a newly allocated Vulkan image or an already existing
/// Vulkan image. The reply holds the produced Texture.
struct MakeTexture : public star::common::IServiceCommandWithReply<star::StarTextures::Texture>
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return make_texture::GetUniqueTypeName();
    }

    MakeTexture(core::device::StarDevice &device, const vk::Image &vulkanImage);

    explicit MakeTexture(core::device::StarDevice &device);

    MakeTexture &setCreateInfo(VmaAllocationCreateInfo nAllocInfo, vk::ImageCreateInfo nCreateInfo,
                               std::string nAllocationName);

    MakeTexture &addViewInfo(const vk::ImageViewCreateInfo &nCreateInfo);

    MakeTexture &setSamplerInfo(const vk::SamplerCreateInfo &nCreateInfo);

    MakeTexture &setBaseFormat(const vk::Format &nFormat);

    MakeTexture &setMipmapLevels(uint32_t nLevels);

    MakeTexture &setSizeInfo(vk::DeviceSize size, vk::Extent3D resolution);

    std::unique_ptr<star::StarTextures::Texture> buildUnique();

    std::shared_ptr<star::StarTextures::Texture> buildShared();

    star::StarTextures::Texture build();

  private:
    core::device::StarDevice &m_device;
    std::optional<vk::Format> m_format = std::nullopt;
    std::optional<star::StarTextures::Texture::Creators> m_createNewAllocationInfo = std::nullopt;
    std::optional<vk::Image> m_vulkanImage = std::nullopt;

    star::StarTextures::TextureViewBuilder m_viewInfos = star::StarTextures::TextureViewBuilder();
    std::optional<vk::SamplerCreateInfo> m_samplerInfo = std::nullopt;
    std::optional<vk::DeviceSize> m_overrideSize = std::nullopt;
    std::optional<vk::Extent3D> m_overrideResolution = std::nullopt;
    uint32_t m_mipmapLevels = 1;
};
} // namespace star::command::texture
