#pragma once

#include "starlight/maker/texture/MakeTextureView.hpp"
#include "starlight/wrappers/graphics/StarTextures/Texture.hpp"

#include <star_common/Maker.hpp>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace star::maker::texture
{
/// @brief Error produced when a texture cannot be created.
struct TextureMakeError
{
    enum class Cause
    {
        unsupported_format,
        invalid_config,
        mip_mismatch,
        out_of_memory,
        device_lost
    };

    Cause cause;
    std::string detail;
    /// @brief Number of textures make() constructed at dst before the failure. Slots [0, built) hold live textures; the
    /// remaining slots hold whatever was there before the call.
    size_t built = 0;
};

/// @brief Builds a texture from either a newly allocated Vulkan image or an already existing Vulkan image.
struct MakeTexture : public star::common::Maker<star::StarTextures::Texture, TextureMakeError>
{
    MakeTexture(core::device::StarDevice &device, const vk::Image &vulkanImage);

    explicit MakeTexture(core::device::StarDevice &device);

    MakeTexture &setCreateInfo(VmaAllocationCreateInfo nAllocInfo, vk::ImageCreateInfo nCreateInfo,
                               std::string nAllocationName);

    MakeTexture &addViewInfo(const vk::ImageViewCreateInfo &nCreateInfo);

    MakeTexture &setSamplerInfo(const vk::SamplerCreateInfo &nCreateInfo);

    MakeTexture &setBaseFormat(const vk::Format &nFormat);

    MakeTexture &setMipmapLevels(uint32_t nLevels);

    MakeTexture &setSizeInfo(vk::DeviceSize size, vk::Extent3D resolution);

    /// @brief Build num textures in place at dst.
    /// @return The number of textures built, or an error whose `built` member reports how many
    ///         textures were constructed at dst before the failure. The maker destroys nothing on error; the
    ///         caller is responsible for destroying the constructed prefix.
    [[nodiscard]] std::expected<size_t, TextureMakeError> make(star::StarTextures::Texture *dst,
                                                               size_t num) noexcept override;

  private:
    star::StarTextures::Texture createTexture();

    star::StarTextures::TextureView buildViews(vk::Image image, const std::string &debugName);
    void buildViewsAndSampler(star::StarTextures::Resources &resources, const std::string &debugName);

    core::device::StarDevice &m_device;
    std::optional<vk::Format> m_format = std::nullopt;
    std::optional<star::StarTextures::Texture::Creators> m_createNewAllocationInfo = std::nullopt;
    std::optional<vk::Image> m_vulkanImage = std::nullopt;

    MakeTextureView m_viewInfos;
    std::optional<vk::SamplerCreateInfo> m_samplerInfo = std::nullopt;
    std::optional<vk::DeviceSize> m_overrideSize = std::nullopt;
    std::optional<vk::Extent3D> m_overrideResolution = std::nullopt;
    uint32_t m_mipmapLevels = 1;
};
} // namespace star::maker::texture
