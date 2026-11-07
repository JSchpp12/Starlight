#pragma once

#include <vulkan/vulkan.hpp>

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <variant>

namespace star::StarTextures
{

/// @brief Tag used by the view containers to represent "no view provided".
struct NoView
{
};

/// @brief Identifies one view of an image.
struct ViewKey
{
    vk::Format format = vk::Format::eUndefined;
    vk::ImageViewType viewType = vk::ImageViewType::e2D;

    bool operator==(const ViewKey &other) const
    {
        return format == other.format && viewType == other.viewType;
    }

    struct Hash
    {
        std::size_t operator()(const ViewKey &key) const noexcept
        {
            const std::uint64_t packed =
                (static_cast<std::uint64_t>(key.format) << 32) | static_cast<std::uint32_t>(key.viewType);
            return std::hash<std::uint64_t>{}(packed);
        }
    };
};

/// @brief Holds exactly one created texture view.
class SingleView
{
  public:
    SingleView() = default;
    SingleView(ViewKey key, vk::ImageView view) : m_key(key), m_view(view)
    {
    }

    const ViewKey &getKey() const
    {
        return m_key;
    }

    vk::ImageView getView() const
    {
        return m_view;
    }

  private:
    ViewKey m_key{};
    vk::ImageView m_view{};
};

/// @brief Holds multiple created views, keyed by {format, view type}.
struct MultipleViews
{
    std::unordered_map<ViewKey, vk::ImageView, ViewKey::Hash> views = {};
    /// Key of the view the texture exposed before it grew additional views. The format-only lookup returns the view
    /// matching the requested format with this view type, so the sampling path keeps binding the original view.
    ViewKey samplingKey = {};
};

/// @brief Storage for a texture's created views.
class TextureView
{
    std::variant<NoView, SingleView, MultipleViews> m_views{NoView{}};

  public:
    TextureView() = default;

    /// Add a created view. The first view is stored inline; a second view promotes the storage to MultipleViews, even
    /// when it shares the format (a different view type is enough).
    TextureView &addView(vk::Format format, vk::ImageViewType viewType, vk::ImageView view);

    /// @brief True when a view exists for the exact format and view type pair.
    bool contains(vk::Format format, vk::ImageViewType viewType) const;

    /// @brief True when any view of this format exists.
    bool contains(vk::Format format) const;

    /// @brief View for the exact format and view type pair, or a null handle when absent.
    vk::ImageView get(vk::Format format, vk::ImageViewType viewType) const;

    /// @brief Sampling view for this format, or a null handle when absent.
    vk::ImageView get(vk::Format format) const;

    /// Destroy every stored view and reset the storage.
    void destroy(vk::Device &device);
};
} // namespace star::StarTextures
