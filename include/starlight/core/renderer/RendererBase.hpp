#pragma once

#include "core/renderer/RenderPhaseConfig.hpp"
#include "systems/StarRenderGroup.hpp"

#include <star_common/IDeviceContext.hpp>

#include <functional>
#include <memory>
#include <optional>
namespace star::core::renderer
{
class RendererBase
{
  public:
    RendererBase() = default;
    RendererBase(core::device::DeviceContext &context, std::vector<std::shared_ptr<StarObject>> objects)
        : m_objects(std::move(objects)) {};
    virtual ~RendererBase() = default;

    virtual void recordPreRenderPassCommands(vk::CommandBuffer &commandBuffer, const common::FrameTracker &ft);
    virtual void recordPostRenderingCalls(vk::CommandBuffer &commandBuffer, const common::FrameTracker &ft);
    virtual void recordRenderingCalls(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                      const uint64_t &frameIndex);
    virtual void recordCommandBuffer(StarCommandBuffer &commandBuffer, const common::FrameTracker &ft,
                                     const uint64_t &frameIndex) = 0;
    virtual void cleanupRender(common::IDeviceContext &context);
    virtual void prepRender(common::IDeviceContext &context);
    virtual void frameUpdate(common::IDeviceContext &context);
    core::device::manager::ManagerCommandBuffer::Request getCommandBufferRequest();

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

  protected:
    /// Optional per-phase submission override. Return a callable to take over
    /// the queue submit (edge-based DAG submission); return nullopt to use the
    /// manager's default order-based submission. Default is nullopt.
    virtual std::optional<core::device::manager::ManagerCommandBuffer::BufferSubmissionOverride> getSubmissionOverride()
    {
        return std::nullopt;
    }

    RenderPhaseConfig m_config;
    std::vector<std::shared_ptr<StarObject>> m_objects;
    std::vector<Handle> m_renderToImages;
    std::vector<Handle> m_renderToDepthImages;
    std::vector<StarRenderGroup> m_renderGroups;
    Handle m_commandBuffer;

    void updateRenderingGroups(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex);

    static std::vector<StarRenderGroup> CreateRenderingGroups(core::device::DeviceContext &context,
                                                              std::vector<std::shared_ptr<StarObject>> objects);
};
} // namespace star::core::renderer