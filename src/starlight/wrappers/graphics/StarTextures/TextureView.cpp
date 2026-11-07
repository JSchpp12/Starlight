#include "StarTextures/TextureView.hpp"

#include <type_traits>
#include <utility>

namespace star::StarTextures
{
TextureView &TextureView::addView(vk::Format format, vk::ImageViewType viewType, vk::ImageView view)
{
    const ViewKey key{format, viewType};

    if (std::holds_alternative<NoView>(m_views))
    {
        // First view for this texture, keep it inline.
        m_views = SingleView{key, view};
    }
    else if (const auto *single = std::get_if<SingleView>(&m_views))
    {
        // Already holding one view, promote to multiple. A second view of the same format with a different view type
        // (e.g. a 2D-array storage view alongside a cube sampling view) lands here too.
        MultipleViews multiple;
        multiple.samplingKey = single->getKey();
        multiple.views.emplace(single->getKey(), single->getView());
        multiple.views.emplace(key, view);
        m_views = std::move(multiple);
    }
    else
    {
        std::get<MultipleViews>(m_views).views.emplace(key, view);
    }

    return *this;
}

bool TextureView::contains(vk::Format format, vk::ImageViewType viewType) const
{
    const ViewKey key{format, viewType};

    return std::visit(
        [&key](const auto &holder) -> bool {
            using T = std::decay_t<decltype(holder)>;
            if constexpr (std::is_same_v<T, NoView>)
            {
                return false;
            }
            else if constexpr (std::is_same_v<T, SingleView>)
            {
                return holder.getKey() == key;
            }
            else
            {
                return holder.views.contains(key);
            }
        },
        m_views);
}

bool TextureView::contains(vk::Format format) const
{
    return std::visit(
        [format](const auto &holder) -> bool {
            using T = std::decay_t<decltype(holder)>;
            if constexpr (std::is_same_v<T, NoView>)
            {
                return false;
            }
            else if constexpr (std::is_same_v<T, SingleView>)
            {
                return holder.getKey().format == format;
            }
            else
            {
                for (const auto &entry : holder.views)
                {
                    if (entry.first.format == format)
                        return true;
                }
                return false;
            }
        },
        m_views);
}

vk::ImageView TextureView::get(vk::Format format, vk::ImageViewType viewType) const
{
    const ViewKey key{format, viewType};

    return std::visit(
        [&key](const auto &holder) -> vk::ImageView {
            using T = std::decay_t<decltype(holder)>;
            if constexpr (std::is_same_v<T, NoView>)
            {
                return vk::ImageView();
            }
            else if constexpr (std::is_same_v<T, SingleView>)
            {
                return holder.getKey() == key ? holder.getView() : vk::ImageView();
            }
            else
            {
                const auto found = holder.views.find(key);
                return found != holder.views.end() ? found->second : vk::ImageView();
            }
        },
        m_views);
}

vk::ImageView TextureView::get(vk::Format format) const
{
    return std::visit(
        [format](const auto &holder) -> vk::ImageView {
            using T = std::decay_t<decltype(holder)>;
            if constexpr (std::is_same_v<T, NoView>)
            {
                return vk::ImageView();
            }
            else if constexpr (std::is_same_v<T, SingleView>)
            {
                return holder.getKey().format == format ? holder.getView() : vk::ImageView();
            }
            else
            {
                // Prefer the view type the format lookup originally returned (the sampling view).
                const ViewKey sampling{format, holder.samplingKey.viewType};
                const auto found = holder.views.find(sampling);
                if (found != holder.views.end())
                    return found->second;

                for (const auto &entry : holder.views)
                {
                    if (entry.first.format == format)
                        return entry.second;
                }
                return vk::ImageView();
            }
        },
        m_views);
}

void TextureView::destroy(vk::Device &device)
{
    std::visit(
        [&device](const auto &holder) {
            using T = std::decay_t<decltype(holder)>;
            if constexpr (std::is_same_v<T, NoView>)
            {
                // Nothing to destroy.
            }
            else if constexpr (std::is_same_v<T, SingleView>)
            {
                if (holder.getView())
                {
                    device.destroyImageView(holder.getView());
                }
            }
            else
            {
                for (const auto &entry : holder.views)
                {
                    if (entry.second)
                    {
                        device.destroyImageView(entry.second);
                    }
                }
            }
        },
        m_views);

    m_views = NoView{};
}
} // namespace star::StarTextures
