#include "StarTextures/TextureViewBuilder.hpp"

#include "starlight/core/Exceptions.hpp"

#include <cassert>
#include <sstream>
#include <type_traits>
#include <utility>

namespace star::StarTextures
{
TextureViewBuilder::TextureViewBuilder(std::vector<vk::ImageViewCreateInfo> infos)
{
    if (infos.size() == 1)
    {
        m_views = SingleView{std::move(infos.front())};
    }
    else if (infos.empty())
    {
        m_views = NoView{};
    }
    else
    {
        m_views = MultipleViews{std::move(infos)};
    }
}

TextureViewBuilder &TextureViewBuilder::add(const vk::ImageViewCreateInfo &info)
{
    if (std::holds_alternative<NoView>(m_views))
    {
        // First view for this texture, keep it inline.
        m_views = SingleView{info};
    }
    else if (std::holds_alternative<SingleView>(m_views))
    {
        // Already holding one view, promote to multiple.
        const SingleView &single = std::get<SingleView>(m_views);

        MultipleViews multiple;
        multiple.infos.reserve(2);
        multiple.infos.push_back(single.info);
        multiple.infos.push_back(info);
        m_views = std::move(multiple);
    }
    else
    {
        std::get<MultipleViews>(m_views).infos.push_back(info);
    }

    return *this;
}

TextureView TextureViewBuilder::build(vk::Device &device, vk::Image image, uint32_t mipmapLevels) const
{
    TextureView textureView;

    auto addView = [&](const vk::ImageViewCreateInfo &viewInfo) {
        if (viewInfo.subresourceRange.levelCount != mipmapLevels)
        {
            std::ostringstream oss;
            oss << "ImageViewCreateInfo levelCount (" << viewInfo.subresourceRange.levelCount
                << ") does not match the texture mipmapLevels (" << mipmapLevels << ")";
            STAR_THROW(oss.str());
        }

        assert(!textureView.contains(viewInfo.format) &&
               "Duplicate format requested for view. Only one view per format is supported");

        vk::ImageViewCreateInfo info = viewInfo;
        info.image = image;

        vk::ImageView view = device.createImageView(info);
        if (!view)
        {
            STAR_THROW("failed to create texture image view!");
        }

        textureView.addView(info.format, view);
    };

    std::visit(
        [&addView](const auto &holder) {
            using T = std::decay_t<decltype(holder)>;
            if constexpr (std::is_same_v<T, NoView>)
            {
                // No views requested.
            }
            else if constexpr (std::is_same_v<T, SingleView>)
            {
                addView(holder.info);
            }
            else
            {
                for (const auto &info : holder.infos)
                {
                    addView(info);
                }
            }
        },
        m_views);

    return textureView;
}
} // namespace star::StarTextures
