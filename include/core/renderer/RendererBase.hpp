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
        : m_renderGroups(CreateRenderingGroups(context, context.getRenderingSurface().getResolution(), objects)) {};
    virtual ~RendererBase() = default;

    virtual void cleanupRender(core::device::DeviceContext &context);
    virtual void prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight);
    virtual void frameUpdate(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex);
    virtual core::device::manager::ManagerCommandBuffer::Request getCommandBufferRequest() = 0;

  protected:
    std::vector<std::unique_ptr<StarRenderGroup>> m_renderGroups;
    Handle m_commandBuffer;

    void updateRenderingGroups(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex);

    static std::vector<std::unique_ptr<StarRenderGroup>> CreateRenderingGroups(
        core::device::DeviceContext &context, const vk::Extent2D &swapChainExtent,
        std::vector<std::shared_ptr<StarObject>> objects);
};
} // namespace star::core::renderer