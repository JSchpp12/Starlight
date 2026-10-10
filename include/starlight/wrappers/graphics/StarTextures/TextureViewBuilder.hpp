#pragma once

#include "StarTextures/TextureView.hpp"

#include <vulkan/vulkan.hpp>

#include <cstdint>
#include <variant>
#include <vector>

namespace star::StarTextures
{

/// @brief Accumulates the create information used to build a texture's image views.
class TextureViewBuilder
{
  public:
    struct SingleView
    {
        vk::ImageViewCreateInfo info = vk::ImageViewCreateInfo();
    };

    struct MultipleViews
    {
        std::vector<vk::ImageViewCreateInfo> infos = std::vector<vk::ImageViewCreateInfo>();
    };

    TextureViewBuilder() = default;

    TextureViewBuilder(const vk::ImageViewCreateInfo &info) : m_views(SingleView{info})
    {
    }

    explicit TextureViewBuilder(std::vector<vk::ImageViewCreateInfo> infos);

    /// @brief  Add a view create info. The first view is stored inline; a second view promotes the storage to
    /// MultipleViews.
    /// @param info
    /// @return
    TextureViewBuilder &add(const vk::ImageViewCreateInfo &info);

    TextureView build(vk::Device &device, vk::Image image, uint32_t mipmapLevels) const;

  private:
    std::variant<NoView, SingleView, MultipleViews> m_views = NoView{};
};
} // namespace star::StarTextures
