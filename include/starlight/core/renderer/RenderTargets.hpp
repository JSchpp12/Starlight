#pragma once

#include "core/renderer/RenderingContext.hpp"

#include <star_common/Handle.hpp>

#include <vulkan/vulkan.hpp>

#include <cstdint>
#include <optional>
#include <vector>

namespace star::core::device
{
class DeviceContext;
}

namespace star::core::renderer
{
class RenderTargets
{
  public:
    static std::vector<StarTextures::Texture> createDefaultColorAttachments(core::device::DeviceContext &context,
                                                                            const size_t numToCreate, int width,
                                                                            int height);
    static std::vector<StarTextures::Texture> createDefaultDepthAttachments(core::device::DeviceContext &context,
                                                                            const size_t numToCreate, int width,
                                                                            int height);
    static RenderTargets forOffscreen(core::device::DeviceContext &context, RenderingContext &renderingContext);
    static std::vector<star::Handle> registerTextures(core::device::DeviceContext &context,
                                                      RenderingContext &renderingContext,
                                                      std::vector<StarTextures::Texture> textures);
    RenderTargets() = default;
    RenderTargets(std::vector<Handle> colorHandles, std::optional<vk::Format> colorFormat,
                  std::vector<Handle> depthHandles, std::optional<vk::Format> depthFormat)
        : m_colorHandles(std::move(colorHandles)), m_colorFormat(colorFormat), m_depthHandles(std::move(depthHandles)),
          m_depthFormat(depthFormat)
    {
    }

    bool hasColor() const
    {
        return !m_colorHandles.empty();
    }
    bool hasDepth() const
    {
        return !m_depthHandles.empty();
    }
    const std::vector<Handle> &colorHandles() const
    {
        return m_colorHandles;
    }
    const std::vector<Handle> &depthHandles() const
    {
        return m_depthHandles;
    }
    std::optional<vk::Format> colorFormat() const
    {
        return m_colorFormat;
    }
    std::optional<vk::Format> depthFormat() const
    {
        return m_depthFormat;
    }

    /// Re-insert the current frame's textures into the rendering context.
    void frameUpdate(core::device::DeviceContext &context, RenderingContext &renderingContext);

  private:
    std::vector<Handle> m_colorHandles, m_depthHandles;
    std::optional<vk::Format> m_colorFormat, m_depthFormat;
};
} // namespace star::core::renderer
