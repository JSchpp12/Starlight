#include "StarTextures/TextureView.hpp"

#include <type_traits>
#include <utility>

namespace star::StarTextures
{
TextureView::TextureView(std::unordered_map<vk::Format, vk::ImageView> views)
{
    if (views.size() == 1)
    {
        const auto &[format, view] = *views.begin();
        m_views = SingleView{format, view};
    }
    else if (views.empty())
    {
        m_views = NoView{};
    }
    else
    {
        m_views = MultipleViews{std::move(views)};
    }
}

TextureView &TextureView::addView(vk::Format format, vk::ImageView view)
{
    if (std::holds_alternative<NoView>(m_views))
    {
        // First view for this texture, keep it inline.
        m_views = SingleView{format, view};
    }
    else if (std::holds_alternative<SingleView>(m_views))
    {
        // Already holding one view, promote to multiple.
        const SingleView &single = std::get<SingleView>(m_views);

        MultipleViews multiple;
        multiple.views.emplace(single.getFormat(), single.getView());
        multiple.views.emplace(format, view);
        m_views = std::move(multiple);
    }
    else
    {
        std::get<MultipleViews>(m_views).views.emplace(format, view);
    }

    return *this;
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
                return holder.getFormat() == format;
            }
            else
            {
                return holder.views.find(format) != holder.views.end();
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
                return holder.getFormat() == format ? holder.getView() : vk::ImageView();
            }
            else
            {
                const auto found = holder.views.find(format);
                return found != holder.views.end() ? found->second : vk::ImageView();
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
