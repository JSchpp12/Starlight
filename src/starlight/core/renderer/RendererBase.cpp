#include "core/renderer/RendererBase.hpp"

#include "core/graphics/GPUWorkSyncInfo.hpp"
#include "starlight/command/command_order/DeclarePass.hpp"
#include "starlight/command/command_order/GetPassInfo.hpp"
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

void RendererBase::recordPreRenderPassCommands(vk::CommandBuffer commandBuffer, const common::FrameTracker &ft)
{
    // One-shot setup records inline, before the pass's own commands. This runs inside the caller's begin()/end().
    if (m_commandBuffers != nullptr)
    {
        const core::graphics::SemaphoreInfo gate =
            m_recordingInfo.has_value() ? m_recordingInfo->gate : core::graphics::SemaphoreInfo{};
        m_commandBuffers->get(m_commandBuffer).recordSetup(commandBuffer, gate);
    }

    if (m_renderGroups.empty())
        return;

    const auto *info = m_recordingInfo.has_value() ? &m_recordingInfo.value() : nullptr;
    for (auto &group : m_renderGroups)
    {
        group.recordPreRenderPassCommands(commandBuffer, ft, ft.getCurrent().getGlobalFrameCounter(), info);
    }
}

void RendererBase::recordRenderingCalls(vk::CommandBuffer commandBuffer, uint8_t frameInFlightIndex,
                                        uint64_t frameIndex)
{
    for (auto &group : m_renderGroups)
    {
        group.recordRenderPassCommands(commandBuffer, frameInFlightIndex, frameIndex);
    }
}

void RendererBase::recordPostRenderingCalls(vk::CommandBuffer commandBuffer, const common::FrameTracker &ft)
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

    // Objects are released above; drain this pass's one-shot work while the app-held GPU contexts are still alive.
    if (m_commandBuffer.isInitialized())
    {
        c.getManagerCommandBuffer().m_manager.get(m_commandBuffer).cleanupSetup(c);
    }
}

void RendererBase::prepRender(common::IDeviceContext &context)
{
    auto &c = static_cast<core::device::DeviceContext &>(context);

    m_renderGroups = CreateRenderingGroups(c, m_objects);

    m_commandBuffer = c.getManagerCommandBuffer().submit(getCommandBufferRequest(),
                                                         c.frameTracker().getCurrent().getGlobalFrameCounter());
    RegisterWithCommandOrder(c.getCmdBus(), c.getEventBus(), c.getGraphicsManagers().queueManager, m_commandBuffer);

    for (auto &group : m_renderGroups)
        group.prepRender(c, m_commandBuffer);
}

void RendererBase::frameUpdate(common::IDeviceContext &context)
{
    auto &c = static_cast<core::device::DeviceContext &>(context);
    updateRenderingGroups(c, c.frameTracker().getCurrent().getFrameInFlightIndex());

    m_commandBuffers = m_commandBuffer.isInitialized() ? &c.getManagerCommandBuffer().m_manager : nullptr;
    m_recordingInfo.reset();

    if (m_commandBuffers != nullptr)
    {
        auto &request = m_commandBuffers->get(m_commandBuffer);
        request.prepareSetup(c);
        // Drain releases from definitions recorded on earlier frames (see RenderPhase::frameUpdate).
        request.pollSetupReleases(c);
        if (!request.isDone())
            m_recordingInfo = core::graphics::GetRecordingSubmissionInfo(c, m_commandBuffer);
    }
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
            if (renderingGroups[j].isObjectCompatible(*objects[i], context))
            {
                match = &renderingGroups[j];
                break;
            }
        }

        if (match != nullptr)
        {
            match->addObject(objects[i], context);
        }
        else
        {
            // create a new one and add object
            renderingGroups.emplace_back(context, objects[i]);
        }
    }

    return renderingGroups;
}

static std::tuple<vk::Semaphore, uint64_t> GetCurrentSyncInfo(const star::Handle &registration,
                                                              const star::core::CommandBus &cmdBus)
{
    auto cmd = star::command_order::GetPassInfo{registration};
    cmdBus.submit(cmd);
    const auto &r = cmd.getReply().get();

    return std::make_tuple(r.signaledSemaphore, r.currentSignalValue);
}

static star::core::graphics::SemaphoreInfo GetTransferRequestSyncToPreviousDraw(const star::Handle &registration,
                                                                                const star::core::CommandBus &cmdBus)
{
    auto [semaphore, currentSignalValue] = GetCurrentSyncInfo(registration, cmdBus);

    return star::core::graphics::SemaphoreInfo{.signalValue = currentSignalValue, .semaphore = semaphore};
}

void RendererBase::updateRenderingGroups(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex)
{
    const auto &transferSyncInfoToUse = GetTransferRequestSyncToPreviousDraw(m_commandBuffer, context.getCmdBus());

    for (auto &group : m_renderGroups)
    {
        group.frameUpdate(context, frameInFlightIndex, m_commandBuffer, transferSyncInfoToUse);
    }
}

core::device::manager::ManagerCommandBuffer::Request RendererBase::getCommandBufferRequest()
{
    return core::device::manager::ManagerCommandBuffer::Request{
        .recordBufferCallback = std::bind(&RendererBase::recordCommandBuffer, this, std::placeholders::_1,
                                          std::placeholders::_2, std::placeholders::_3),
        .order = m_config.order,
        .orderIndex = m_config.orderIndex,
        .type = m_config.queueType,
        .waitStage = m_config.waitStage,
        .willBeSubmittedEachFrame = m_config.willBeSubmittedEachFrame,
        .recordOnce = m_config.recordOnce,
        .overrideBufferSubmissionCallback = getSubmissionOverride(),
    };
}
} // namespace star::core::renderer