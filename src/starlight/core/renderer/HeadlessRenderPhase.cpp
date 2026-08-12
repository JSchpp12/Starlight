#include "starlight/core/renderer/HeadlessRenderPhase.hpp"

#include "starlight/command/command_order/GetPassInfo.hpp"
#include "starlight/command/command_order/TriggerPass.hpp"
#include "starlight/core/renderer/EdgeSubmission.hpp"

namespace star::core::renderer
{
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
    core::device::manager::ManagerCommandBuffer::BufferSubmissionOverride overrideFn = std::bind(
        &HeadlessRenderPhase::submitBuffer, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
        std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7);
    return overrideFn;
}

void HeadlessRenderPhase::waitForSemaphore(const common::FrameTracker &ft) const
{
    uint64_t signalValue{0};
    vk::Semaphore semaphore{VK_NULL_HANDLE};
    {
        star::command_order::GetPassInfo get{m_commandBuffer};
        m_cmdBus->submit(get);
        signalValue = get.getReply().get().currentSignalValue;
        semaphore = get.getReply().get().signaledSemaphore;
    }

    const uint64_t frameCount = ft.getCurrent().getNumTimesFrameProcessed();
    if (frameCount == signalValue)
    {
        auto result =
            m_device.waitSemaphores(vk::SemaphoreWaitInfo().setValues(frameCount).setSemaphores(semaphore), UINT64_MAX);

        if (result != vk::Result::eSuccess)
        {
            STAR_THROW("Failed to wait for timeline semaphores");
        }
    }
}

void HeadlessRenderPhase::recordCommandBuffer(StarCommandBuffer &commandBuffer, const common::FrameTracker &ft,
                                              const uint64_t &frameIndex)
{
    waitForSemaphore(ft);

    this->DefaultRenderPhase::recordCommandBuffer(commandBuffer, ft, frameIndex);
}

vk::Semaphore HeadlessRenderPhase::submitBuffer(StarCommandBuffer &buffer, const common::FrameTracker &frameTracker,
                                                std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                                                std::vector<vk::Semaphore> dataSemaphores,
                                                std::vector<vk::PipelineStageFlags> dataWaitPoints,
                                                std::vector<std::optional<uint64_t>> previousSignaledValues,
                                                StarQueue &queue)
{
    return submitEdgeAwarePass(*m_cmdBus, m_commandBuffer, buffer, frameTracker, previousCommandBufferSemaphores,
                               dataSemaphores, dataWaitPoints, previousSignaledValues, queue,
                               /*signalBinaryCompletion=*/true);
}
} // namespace star::core::renderer
