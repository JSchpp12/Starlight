#pragma once

#include "core/renderer/RenderingContext.hpp"

#include <star_common/Handle.hpp>

#include <vulkan/vulkan.hpp>

#include <cstdint>
#include <vector>

namespace star::core::device
{
class DeviceContext;
}

namespace star::core::renderer
{
/// Per-frame-in-flight color + depth images a phase renders into, plus their
/// registration into the rendering context. Created once via a provider function
/// pointer selected at construction (forPresented / forOffscreen), so the phase
/// carries no virtual for render-target creation.
class RenderTargets
{
  public:
    RenderTargets() = default;

    using Provider = RenderTargets (*)(core::device::DeviceContext &context, RenderingContext &renderingContext);

    static RenderTargets forPresentation(core::device::DeviceContext &context, RenderingContext &renderingContext);
    static RenderTargets forOffscreen(core::device::DeviceContext &context, RenderingContext &renderingContext);

    const std::vector<Handle> &colorHandles() const
    {
        return m_colorHandles;
    }
    const std::vector<Handle> &depthHandles() const
    {
        return m_depthHandles;
    }
    vk::Format colorFormat() const
    {
        return m_colorFormat;
    }
    vk::Format depthFormat() const
    {
        return m_depthFormat;
    }

    /// Re-insert the current frame's textures into the rendering context.
    void frameUpdate(core::device::DeviceContext &context, RenderingContext &renderingContext);

    /// Assemble from handles that were registered by the caller. Used by the
    /// virtual-fallback creation path in DefaultRenderer, which registers images
    /// itself then hands the handles here so frameUpdate has a consistent place
    /// to read them.
    RenderTargets(std::vector<Handle> colorHandles, vk::Format colorFormat, std::vector<Handle> depthHandles,
                  vk::Format depthFormat)
        : m_colorHandles(std::move(colorHandles)), m_colorFormat(colorFormat),
          m_depthHandles(std::move(depthHandles)), m_depthFormat(depthFormat)
    {
    }

    /// Register a batch of built textures with the image manager and insert each
    /// handle->texture* into the rendering context. Returns the assigned handles
    /// (the image manager owns the textures).
    static std::vector<Handle> registerTextures(core::device::DeviceContext &context,
                                                 RenderingContext &renderingContext,
                                                 std::vector<StarTextures::Texture> textures);

  private:
    std::vector<Handle> m_colorHandles, m_depthHandles;
    vk::Format m_colorFormat{vk::Format::eUndefined}, m_depthFormat{vk::Format::eUndefined};
};
} // namespace star::core::renderer