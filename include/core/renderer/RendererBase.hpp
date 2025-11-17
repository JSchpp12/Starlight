#pragma once

#include "systems/StarRenderGroup.hpp"

#include <memory>
namespace star::core::renderer
{
class RendererBase
{
  public:
    RendererBase(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                 std::vector<std::shared_ptr<StarObject>> objects)
        : m_renderGroups(CreateRenderingGroups(context, context.getRenderingSurface().getResolution(), objects)){};
    virtual ~RendererBase() = default;

    virtual void recordPreRenderPassCommands(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                             const uint64_t &frameIndex);
    virtual void recordPostRenderingCalls(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex);
    virtual void recordRenderingCalls(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                      const uint64_t &frameIndex);
    virtual void cleanupRender(core::device::DeviceContext &context);
    virtual void prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight);
    virtual void frameUpdate(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex);
    virtual core::device::manager::ManagerCommandBuffer::Request getCommandBufferRequest() = 0;

    Handle getCommandBuffer() const
    {
        return m_commandBuffer;
    }

    Handle &getCommandBuffer()
    {
        return m_commandBuffer;
    }

  protected:
    std::vector<std::unique_ptr<StarRenderGroup>> m_renderGroups;
    Handle m_commandBuffer;

    void updateRenderingGroups(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex);

    void prepRenderingGroups(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex);

    static std::vector<std::unique_ptr<StarRenderGroup>> CreateRenderingGroups(
        core::device::DeviceContext &context, const vk::Extent2D &swapChainExtent,
        std::vector<std::shared_ptr<StarObject>> objects);
};
} // namespace star::core::renderer