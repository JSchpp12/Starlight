#pragma once

#include "starlight/wrappers/graphics/StarTextures/TextureView.hpp"

#include <star_common/Maker.hpp>

#include <vulkan/vulkan.hpp>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <vector>

namespace star::core::device
{
class StarDevice;
} // namespace star::core::device

namespace star::maker::texture
{
/// @brief Error produced when a texture view cannot be created.
struct TextureViewMakeError
{
    enum class Cause
    {
        invalid_config,
        out_of_memory,
        device_lost
    };

    Cause cause;
    std::string detail;
    /// @brief Number of texture views make() constructed at dst before the failure. Slots [0, built) hold live views;
    /// the remaining slots hold whatever was there before the call.
    size_t built = 0;
};

/// @brief Accumulates image view create information and builds TextureView objects for a given image.
class MakeTextureView : public star::common::Maker<star::StarTextures::TextureView, TextureViewMakeError>
{
  public:
    explicit MakeTextureView(core::device::StarDevice &device);

    /// @brief Add a view create info. Views are kept in insertion order.
    MakeTextureView &addViewInfo(const vk::ImageViewCreateInfo &info);

    /// @brief Set the image the views will reference.
    MakeTextureView &setImage(vk::Image image);

    /// @brief Set the number of mip levels each view must cover.
    MakeTextureView &setMipmapLevels(uint32_t mipmapLevels);

    /// @brief Set the base name used to label the created views for the validation layers.
    MakeTextureView &setDebugName(std::string debugName);

    /// @brief Build num texture views in place at dst.
    /// @return The number of views built, or an error whose `built` member reports how many views were constructed
    ///         at dst before the failure.
    [[nodiscard]] std::expected<size_t, TextureViewMakeError> make(star::StarTextures::TextureView *dst,
                                                                   size_t num) noexcept override;

  private:
    star::StarTextures::TextureView createView();

    core::device::StarDevice &m_device;
    vk::Image m_image{VK_NULL_HANDLE};
    uint32_t m_mipmapLevels = 1;
    std::string m_debugName;

    std::vector<vk::ImageViewCreateInfo> m_views;
};
} // namespace star::maker::texture
