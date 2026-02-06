#pragma once

#include "systems/StarRenderGroup.hpp"

#include <star_common/IDeviceContext.hpp>

#include <memory>
namespace star::core::renderer
{
class RendererBase
{
  public:
    RendererBase() = default;
    RendererBase(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                 std::vector<std::shared_ptr<StarObject>> objects)
        : m_objects(std::move(objects)){};
    virtual ~RendererBase() = default;

    virtual void recordPreRenderPassCommands(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                             const uint64_t &frameIndex);
    virtual void recordPostRenderingCalls(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex);
    virtual void recordRenderingCalls(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                      const uint64_t &frameIndex);
    virtual void cleanupRender(common::IDeviceContext &context);
    virtual void prepRender(common::IDeviceContext &context);
    virtual void frameUpdate(common::IDeviceContext &context);
    virtual core::device::manager::ManagerCommandBuffer::Request getCommandBufferRequest() = 0;

    Handle getCommandBuffer() const
    {
        return m_commandBuffer;
    }

    Handle &getCommandBuffer()
    {
        return m_commandBuffer;
    }
    std::vector<Handle> &getRenderToColorImages()
    {
        return m_renderToImages;
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
    std::vector<std::shared_ptr<StarObject>> m_objects;
    std::vector<Handle> m_renderToImages;
    std::vector<Handle> m_renderToDepthImages;
    std::vector<StarRenderGroup> m_renderGroups;
    Handle m_commandBuffer;

    void updateRenderingGroups(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex);

    void prepRenderingGroups(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex);

    static std::vector<StarRenderGroup> CreateRenderingGroups(core::device::DeviceContext &context,
                                                              std::vector<std::shared_ptr<StarObject>> objects);
};
} // namespace star::core::renderer