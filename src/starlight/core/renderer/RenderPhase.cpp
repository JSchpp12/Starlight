#include "core/renderer/RenderPhase.hpp"
#include "core/device/DeviceContext.hpp"

#include "starlight/command/command_order/GetPassInfo.hpp"
#include "starlight/core/graphics/GPUWorkSyncInfo.hpp"

#include <star_common/Handle.hpp>

#include <cassert>
#include <tuple>

namespace star::core::renderer
{
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

void RenderPhase::recordPreRenderPassCommands(vk::CommandBuffer commandBuffer, const common::FrameTracker &ft)
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

    // Resolved in frameUpdate; recording only consumes the gate.
    const auto *info = m_recordingInfo.has_value() ? &m_recordingInfo.value() : nullptr;
    for (auto &group : m_renderGroups)
    {
        group.recordPreRenderPassCommands(commandBuffer, ft, ft.getCurrent().getGlobalFrameCounter(), info);
    }
}

void RenderPhase::recordRenderingCalls(vk::CommandBuffer commandBuffer, uint8_t frameInFlightIndex, uint64_t frameIndex)
{
    for (auto &group : m_renderGroups)
    {
        group.recordRenderPassCommands(commandBuffer, frameInFlightIndex, frameIndex);
    }
}

void RenderPhase::recordPostRenderingCalls(vk::CommandBuffer commandBuffer, const common::FrameTracker &ft)
{
    for (auto &group : m_renderGroups)
    {
        group.recordPostRenderPassCommands(commandBuffer, ft.getCurrent().getFrameInFlightIndex());
    }
}

void RenderPhase::cleanupRender(common::IDeviceContext &context)
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

void RenderPhase::frameUpdate(common::IDeviceContext &context)
{
    auto &c = static_cast<core::device::DeviceContext &>(context);
    updateRenderingGroups(c, c.frameTracker().getCurrent().getFrameInFlightIndex());

    // CommandOrderService already prepared and added one-shot work during this frame's TriggerPass. Resolve the gate
    // only while the pass still has work that has not recorded. The pass trigger is established before scene
    // frameUpdate, so the gate is valid here.
    m_commandBuffers = m_commandBuffer.isInitialized() ? &c.getManagerCommandBuffer().m_manager : nullptr;
    m_recordingInfo.reset();

    if (m_commandBuffers != nullptr)
    {
        auto &request = m_commandBuffers->get(m_commandBuffer);
        request.prepareSetup(c);
        // Drain releases from definitions recorded on earlier frames; this is what actually runs a per-make
        // keepAlive (e.g. the skybox conversion set) while the app is running.
        request.pollSetupReleases(c);
        if (!request.isDone())
            m_recordingInfo = core::graphics::GetRecordingSubmissionInfo(c, m_commandBuffer);
    }
}

void RenderPhase::updateRenderingGroups(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex)
{
    const auto &transferSyncInfoToUse = GetTransferRequestSyncToPreviousDraw(m_commandBuffer, context.getCmdBus());

    for (auto &group : m_renderGroups)
    {
        group.frameUpdate(context, frameInFlightIndex, m_commandBuffer, transferSyncInfoToUse);
    }
}

void RenderPhase::updateDependentData(core::device::DeviceContext &context)
{
    if (!m_drivesFrameData)
        return;

    auto result = m_frameData->frameUpdate(context);
    auto &record = context.getManagerCommandBuffer().m_manager.get(m_commandBuffer);
    for (const auto &w : result.waits)
    {
        record.oneTimeWaitSemaphoreInfo.insert(w.handle, w.semaphore, w.waitStage, w.signalValue);
        m_renderingContext.addBufferToRenderingContext(context, w.handle);
    }
}
} // namespace star::core::renderer