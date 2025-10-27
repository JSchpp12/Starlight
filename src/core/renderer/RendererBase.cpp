#include "core/renderer/RendererBase.hpp"

namespace star::core::renderer
{

void RendererBase::recordPreRenderPassCommands(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                               const uint64_t &frameIndex)
{
    for (auto &group : m_renderGroups)
    {
        group->recordPreRenderPassCommands(commandBuffer, frameInFlightIndex, frameIndex);
    }
}

void RendererBase::recordRenderingCalls(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                        const uint64_t &frameIndex)
{
    for (auto &group : m_renderGroups)
    {
        group->recordRenderPassCommands(commandBuffer, frameInFlightIndex, frameIndex);
    }
}

void RendererBase::recordPostRenderingCalls(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex)
{
    for (auto &group : m_renderGroups)
    {
        group->recordPostRenderPassCommands(commandBuffer, frameInFlightIndex);
    }
}

void RendererBase::cleanupRender(core::device::DeviceContext &context)
{
    for (size_t i = 0; i < m_renderGroups.size(); i++)
    {
        m_renderGroups[i]->cleanupRender(context);
    }
}

void RendererBase::prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight)
{
    m_commandBuffer =
        context.getManagerCommandBuffer().submit(getCommandBufferRequest(), context.getCurrentFrameIndex());
}

void RendererBase::frameUpdate(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex)
{
    updateRenderingGroups(context, frameInFlightIndex);
}

std::vector<std::unique_ptr<star::StarRenderGroup>> RendererBase::CreateRenderingGroups(
    core::device::DeviceContext &context, const vk::Extent2D &swapChainExtent,
    std::vector<std::shared_ptr<StarObject>> objects)
{
    auto renderingGroups = std::vector<std::unique_ptr<StarRenderGroup>>();

    for (size_t i = 0; i < objects.size(); i++)
    {
        // check if the object is compatible with any render groups
        StarRenderGroup *match = nullptr;

        // if it is not, create a new render group
        for (size_t j = 0; j < renderingGroups.size(); j++)
        {
            if (renderingGroups[j]->isObjectCompatible(*objects[i]))
            {
                match = renderingGroups[j].get();
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
            renderingGroups.emplace_back(std::unique_ptr<StarRenderGroup>(new StarRenderGroup(context, objects[i])));
        }
    }

    return renderingGroups;
}

void RendererBase::updateRenderingGroups(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex)
{
    for (auto &group : m_renderGroups)
    {
        group->frameUpdate(context, frameInFlightIndex, m_commandBuffer);
    }
}
} // namespace star::core::renderer