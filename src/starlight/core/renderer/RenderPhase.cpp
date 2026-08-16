#include "core/renderer/RenderPhase.hpp"
#include "core/device/DeviceContext.hpp"

#include "starlight/command/command_order/GetPassInfo.hpp"
#include "starlight/core/graphics/GPUWorkSyncInfo.hpp"

#include <star_common/Handle.hpp>

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

void RenderPhase::recordPreRenderPassCommands(vk::CommandBuffer &commandBuffer, const common::FrameTracker &ft)
{
    for (auto &group : m_renderGroups)
    {
        group.recordPreRenderPassCommands(commandBuffer, ft.getCurrent().getFrameInFlightIndex(),
                                          ft.getCurrent().getGlobalFrameCounter());
    }
}

void RenderPhase::recordRenderingCalls(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                       const uint64_t &frameIndex)
{
    for (auto &group : m_renderGroups)
    {
        group.recordRenderPassCommands(commandBuffer, frameInFlightIndex, frameIndex);
    }
}

void RenderPhase::recordPostRenderingCalls(vk::CommandBuffer &commandBuffer, const common::FrameTracker &ft)
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
}

void RenderPhase::frameUpdate(common::IDeviceContext &context)
{
    auto &c = static_cast<core::device::DeviceContext &>(context);
    updateRenderingGroups(c, c.frameTracker().getCurrent().getFrameInFlightIndex());
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