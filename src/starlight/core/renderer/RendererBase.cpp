#include "core/renderer/RendererBase.hpp"

#include "starlight/command/command_order/DeclarePass.hpp"
#include "starlight/core/helper/queue/QueueHelpers.hpp"

#include <star_common/EventBus.hpp>
#include <star_common/Handle.hpp>

namespace star::core::renderer
{

static void RegisterWithCommandOrder(const star::core::CommandBus &cmdBus, star::common::EventBus &evtBus,
                                     star::core::device::manager::Queue &qm, Handle commandBuffer)
{
    auto *queue = star::core::helper::GetEngineDefaultQueue(evtBus, qm, star::Queue_Type::Tgraphics);
    assert(queue != nullptr && "Failed to acquire default engine queue");

    cmdBus.submit(star::command_order::DeclarePass{std::move(commandBuffer), queue->getParentQueueFamilyIndex()});
}

void RendererBase::recordPreRenderPassCommands(vk::CommandBuffer &commandBuffer, const common::FrameTracker &ft)
{
    for (auto &group : m_renderGroups)
    {
        group.recordPreRenderPassCommands(commandBuffer, ft.getCurrent().getFrameInFlightIndex(),
                                          ft.getCurrent().getGlobalFrameCounter());
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

void RendererBase::recordPostRenderingCalls(vk::CommandBuffer &commandBuffer, const common::FrameTracker &ft)
{
    for (auto &group : m_renderGroups)
    {
        group.recordPostRenderPassCommands(commandBuffer, ft.getCurrent().getFrameInFlightIndex());
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

    m_commandBuffer = c.getManagerCommandBuffer().submit(getCommandBufferRequest(),
                                                         c.getFrameTracker().getCurrent().getGlobalFrameCounter());
    RegisterWithCommandOrder(c.getCmdBus(), c.getEventBus(), c.getGraphicsManagers().queueManager, m_commandBuffer);
}

void RendererBase::frameUpdate(common::IDeviceContext &context)
{
    auto &c = static_cast<core::device::DeviceContext &>(context);
    updateRenderingGroups(c, c.getFrameTracker().getCurrent().getFrameInFlightIndex());
}

std::vector<star::StarRenderGroup> RendererBase::CreateRenderingGroups(core::device::DeviceContext &context,
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