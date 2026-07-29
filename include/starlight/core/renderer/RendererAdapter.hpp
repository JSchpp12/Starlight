#pragma once

#include "core/renderer/IRenderPhaseProvider.hpp"
#include "core/renderer/RenderPhase.hpp"
#include "core/renderer/RendererBase.hpp"

#include <star_common/Renderer.hpp>

#include <memory>
#include <utility>

namespace star::core::renderer
{
/// RenderPhase that owns and delegates to an existing (not-yet-split)
/// common::Renderer. Transitional: it lets the scene's new provider/phase path
/// drive a renderer that has not yet been split into a real provider +
/// RenderPhase. The wrapped renderer keeps its own command buffer and
/// recording (registered at its own prepRender); this phase only forwards the
/// per-frame lifecycle the scene drives (frameUpdate, cleanupRender).
class RendererAdapterPhase : public RenderPhase
{
  public:
    explicit RendererAdapterPhase(common::Renderer renderer) : m_renderer(std::move(renderer))
    {
    }

    void frameUpdate(common::IDeviceContext &context) override
    {
        m_renderer.frameUpdate(context);
    }

    void cleanupRender(common::IDeviceContext &context) override
    {
        m_renderer.cleanupRender(context);
    }

    void recordCommandBuffer(StarCommandBuffer &commandBuffer, const common::FrameTracker &ft,
                             const uint64_t &frameIndex) override
    {
        // The wrapped renderer's own command buffer is the one registered with
        // the command order, so this is not on the hot path; keep it delegating
        // for completeness.
        if (auto *base = m_renderer.getRawBase())
            base->recordCommandBuffer(commandBuffer, ft, frameIndex);
    }

  private:
    common::Renderer m_renderer;
};

/// Transient IRenderPhaseProvider that hands an existing (not-yet-split)
/// renderer to the scene. build() runs the renderer's own prepRender -- its full
/// setup, including command-buffer registration -- and returns a
/// RendererAdapterPhase that owns it. The provider itself is discardable after
/// build(); the produced phase (owning the renderer) persists in the scene.
class RendererAdapter : public IRenderPhaseProvider
{
  public:
    explicit RendererAdapter(common::Renderer renderer) : m_renderer(std::move(renderer))
    {
    }

    std::unique_ptr<RenderPhase> build(core::device::DeviceContext &context) override
    {
        m_renderer.prepRender(context);
        return std::make_unique<RendererAdapterPhase>(std::move(m_renderer));
    }

  private:
    common::Renderer m_renderer;
};
} // namespace star::core::renderer