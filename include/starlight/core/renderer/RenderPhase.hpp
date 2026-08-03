#pragma once

#include "core/renderer/FrameData.hpp"
#include "core/renderer/RenderTargets.hpp"
#include "core/renderer/RenderingTargetInfo.hpp"
#include "systems/StarRenderGroup.hpp"

#include <star_common/FrameTracker.hpp>
#include <star_common/IDeviceContext.hpp>

#include <memory>
#include <optional>
#include <vector>

namespace star::core::renderer
{
/// Runtime half of a renderer. A RenderPhase is built once by an
/// IRenderPhaseProvider at prep time and then driven every frame: it owns the
/// command buffer, the phase's draw-time targets (RenderTargets), the rendering
/// context, and the render groups it records over. All one-shot setup --
/// target creation, group building, command-buffer request -- lives on the
/// provider, not here. Recording stays virtual for now; the last step in the
/// overall plan replaces it with data-driven DrawCommands.
class RenderPhase
{
  public:
    RenderPhase() = default;
    virtual ~RenderPhase() = default;

    /// Per-frame update of this phase's dependent data and render groups.
    virtual void frameUpdate(common::IDeviceContext &context);

    /// Record this phase's commands for the frame. Each phase kind implements.
    virtual void recordCommandBuffer(StarCommandBuffer &commandBuffer, const common::FrameTracker &ft,
                                     const uint64_t &frameIndex) = 0;

    /// Release per-phase resources (render groups, ...).
    virtual void cleanupRender(common::IDeviceContext &context);

    /// Recording helpers that iterate the render groups; phases may override
    /// for custom pre/post-pass work.
    virtual void recordPreRenderPassCommands(vk::CommandBuffer &commandBuffer, const common::FrameTracker &ft);
    virtual void recordRenderingCalls(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                      const uint64_t &frameIndex);
    virtual void recordPostRenderingCalls(vk::CommandBuffer &commandBuffer, const common::FrameTracker &ft);

    const Handle &getCommandBuffer() const
    {
        return m_commandBuffer;
    }
    const std::vector<Handle> &getRenderToColorImages() const
    {
        return m_renderToImages;
    }
    std::vector<Handle> &getRenderToDepthImages()
    {
        return m_renderToDepthImages;
    }
    const std::vector<Handle> &getRenderToDepthImages() const
    {
        return m_renderToDepthImages;
    }
    std::vector<std::shared_ptr<StarObject>> &getObjects()
    {
        return m_objects;
    }
    const std::vector<std::shared_ptr<StarObject>> &getObjects() const
    {
        return m_objects;
    }
    std::shared_ptr<FrameData> getFrameData()
    {
        return m_frameData;
    }
    const RenderTargets &getRenderTargets() const
    {
        return m_renderTargets;
    }

    virtual RenderingTargetInfo getRenderTargetInfo() const
    {
        return RenderingTargetInfo({m_colorFormat}, m_depthFormat);
    }

  protected:
    /// Per-phase submission override (edge-based DAG submit). Read by the
    /// provider when it builds the command-buffer request. Default: none.
    virtual std::optional<core::device::manager::ManagerCommandBuffer::BufferSubmissionOverride> getSubmissionOverride()
    {
        return std::nullopt;
    }

    void updateRenderingGroups(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex);

    RenderTargets m_renderTargets;
    RenderingContext m_renderingContext;
    std::shared_ptr<FrameData> m_frameData;
    std::vector<std::shared_ptr<StarObject>> m_objects;
    std::vector<Handle> m_renderToImages;
    std::vector<Handle> m_renderToDepthImages;
    std::vector<StarRenderGroup> m_renderGroups;
    Handle m_commandBuffer;
    vk::Format m_colorFormat{vk::Format::eUndefined};
    vk::Format m_depthFormat{vk::Format::eUndefined};
};
} // namespace star::core::renderer