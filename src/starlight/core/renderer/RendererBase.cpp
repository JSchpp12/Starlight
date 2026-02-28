#include "core/renderer/RendererBase.hpp"

namespace star::core::renderer
{

void RendererBase::recordPreRenderPassCommands(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                               const uint64_t &frameIndex)
{
    for (auto &group : m_renderGroups)
    {
        group.recordPreRenderPassCommands(commandBuffer, frameInFlightIndex, frameIndex);
    }
}

void RendererBase::recordRenderingCalls(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                        const uint64_t &frameIndex)
{
    for (auto &group : m_renderGroups)
    {
        group.recordRenderPassCommands(commandBuffer, frameInFlightIndex, frameIndex);
    }
}

void RendererBase::recordPostRenderingCalls(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex)
{
    for (auto &group : m_renderGroups)
    {
        group.recordPostRenderPassCommands(commandBuffer, frameInFlightIndex);
    }
}

void RendererBase::cleanupRender(common::IDeviceContext &context)
{
    auto &c = static_cast<core::device::DeviceContext &>(context); 

    for (size_t i = 0; i < m_renderGroups.size(); i++)
    {
        m_renderGroups[i].cleanupRender(c);
    }
}

void RendererBase::prepRender(common::IDeviceContext &context)
{
    auto &c = static_cast<core::device::DeviceContext &>(context);

    m_renderGroups = CreateRenderingGroups(c, m_objects);

    m_commandBuffer =
        c.getManagerCommandBuffer().submit(getCommandBufferRequest(), c.getFrameTracker().getCurrent().getGlobalFrameCounter());
}

void RendererBase::frameUpdate(common::IDeviceContext &context)
{
    auto &c = static_cast<core::device::DeviceContext &>(context);
    updateRenderingGroups(c, c.getFrameTracker().getCurrent().getFrameInFlightIndex());
}

std::vector<star::StarRenderGroup> RendererBase::CreateRenderingGroups(
    core::device::DeviceContext &context,
    std::vector<std::shared_ptr<StarObject>> objects)
{
    auto renderingGroups = std::vector<StarRenderGroup>();

    for (size_t i = 0; i < objects.size(); i++)
    {
        // check if the object is compatible with any render groups
        StarRenderGroup *match = nullptr;

        // if it is not, create a new render group
        for (size_t j = 0; j < renderingGroups.size(); j++)
        {
            if (renderingGroups[j].isObjectCompatible(*objects[i]))
            {
                match = &renderingGroups[j];
                break;
            }
        }

        if (match != nullptr)
        {
            match->addObject(objects[i]);
        }
        else
        {
            // create a new one and add object
            renderingGroups.emplace_back(context, objects[i]);
        }
    }

    return renderingGroups;
}

void RendererBase::updateRenderingGroups(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex)
{
    for (auto &group : m_renderGroups)
    {
        group.frameUpdate(context, frameInFlightIndex, m_commandBuffer);
    }
}
} // namespace star::core::renderer