#pragma once

#include <vulkan/vulkan.hpp>

#include <unordered_map>
#include <variant>

namespace star::StarTextures
{

/// @brief Tag used by the view containers to represent "no view provided".
struct NoView
{
};

/// @brief Holds exactly one created texture view.
class SingleView
{
  public:
    SingleView() = default;
    SingleView(vk::Format format, vk::ImageView view) : m_view{format, view}
    {
    }

    vk::Format getFormat() const
    {
        return m_view.format;
    }

    vk::ImageView getView() const
    {
        return m_view.view;
    }

  private:
    struct ViewInfo
    {
        vk::Format format{vk::Format::eUndefined};
        vk::ImageView view{};
    };

    ViewInfo m_view{};
};

/// @brief Holds multiple created texture views in the same map
struct MultipleViews
{
    std::unordered_map<vk::Format, vk::ImageView> views = std::unordered_map<vk::Format, vk::ImageView>();
};

/// @brief Storage for a texture's created views.
class TextureView
{
    std::variant<NoView, SingleView, MultipleViews> m_views{NoView{}};

  public:
    TextureView() = default;

    TextureView(vk::Format format, vk::ImageView view) : m_views(SingleView{format, view})
    {
    }

    explicit TextureView(std::unordered_map<vk::Format, vk::ImageView> views);

    /// Add a created view. The first view is stored inline; a second view
    /// promotes the storage to MultipleViews.
    TextureView &addView(vk::Format format, vk::ImageView view);

    bool contains(vk::Format format) const;

    /// Returns the view for the requested format, or a null handle if it is not
    /// present.
    vk::ImageView get(vk::Format format) const;

    /// Destroy every stored view and reset the storage.
    void destroy(vk::Device &device);
};
} // namespace star::StarTextures
