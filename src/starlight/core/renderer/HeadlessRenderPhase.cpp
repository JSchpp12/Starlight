#include "starlight/core/renderer/HeadlessRenderPhase.hpp"

#include "starlight/command/command_order/TriggerPass.hpp"
#include "starlight/core/renderer/RenderPhaseHelpers.hpp"

namespace star::core::renderer
{
HeadlessRenderPhase::HeadlessRenderPhase(const star::core::CommandBus &cmdBus, vk::Device device)
    : m_device(device), m_cmdBus(&cmdBus)
{
}

namespace pre_pass
{
vk::ImageMemoryBarrier2 GetImageFromNeighbor::getBarrier() const noexcept
{
    assert(targetTexture != nullptr && "The target texture must be provided during init");
    return vk::ImageMemoryBarrier2();
}
} // namespace pre_pass

namespace post_pass
{
vk::ImageMemoryBarrier2 PrepImageForNeighbor::getBarrier() const noexcept
{
    assert(targetTexture != nullptr && "The target texture must be provided during init");
    return vk::ImageMemoryBarrier2();
}
} // namespace post_pass

void HeadlessRenderPhase::frameUpdate(common::IDeviceContext &c)
{
    auto &context = static_cast<star::core::device::DeviceContext &>(c);
    const size_t ii = static_cast<size_t>(context.frameTracker().getCurrent().getFrameInFlightIndex());

    context.getCmdBus().submit(star::command_order::TriggerPass()
                                   .setTimelineSemaphore(m_timelineSemaphores[ii])
                                   .setSignalValue(context.frameTracker().getCurrent().getNumTimesFrameProcessed() + 1)
                                   .setPass(m_commandBuffer));

    this->DefaultRenderPhase::frameUpdate(c);
}

std::optional<core::device::manager::ManagerCommandBuffer::BufferSubmissionOverride> HeadlessRenderPhase::
    getSubmissionOverride()
{
    return makeEdgeAwareSubmissionOverride(m_cmdBus, &m_commandBuffer, /*signalBinaryCompletion=*/true);
}

void HeadlessRenderPhase::recordCommandBuffer(StarCommandBuffer &commandBuffer, const common::FrameTracker &ft,
                                              const uint64_t &frameIndex)
{
    waitForTimelineSemaphore(*m_cmdBus, m_device, m_commandBuffer, ft);

    this->DefaultRenderPhase::recordCommandBuffer(commandBuffer, ft, frameIndex);
}
} // namespace star::core::renderer
