#include "starlight/core/renderer/EdgeSubmission.hpp"

#include "StarCommandBuffer.hpp"
#include "StarQueue.hpp"
#include "starlight/command/command_order/GetPassInfo.hpp"
#include "starlight/core/CommandBus.hpp"

#include <boost/container/small_vector.hpp>

#include <cassert>

namespace star::core::renderer
{
constexpr size_t k_waitInfoCacheSize = 8;

vk::Semaphore submitEdgeAwarePass(const star::core::CommandBus &cmdBus, star::Handle commandBuffer,
                                  star::StarCommandBuffer &buffer, const star::common::FrameTracker &frameTracker,
                                  std::vector<vk::Semaphore> * /*previousCommandBufferSemaphores*/,
                                  std::span<const star::StarCommandBuffer::Data> dataWaits, star::StarQueue &queue,
                                  bool signalBinaryCompletion)
{
    const size_t ii = static_cast<size_t>(frameTracker.getCurrent().getFrameInFlightIndex());
    boost::container::small_vector<vk::SemaphoreSubmitInfo, k_waitInfoCacheSize> waitInfo;
    vk::Semaphore mySemaphore{VK_NULL_HANDLE};
    uint64_t mySignalValue{0};

    // Neighbor waits discovered through the command-order DAG.
    {
        auto cmd = star::command_order::GetPassInfo{commandBuffer};
        cmdBus.submit(cmd);
        const auto &reply = cmd.getReply().get();
        mySemaphore = reply.signaledSemaphore;
        mySignalValue = reply.toSignalValue;

        if (reply.edges != nullptr)
        {
            for (const auto &edge : *reply.edges)
            {
                if (edge.producer == commandBuffer)
                {
                    // We are the producer: wait for the consumer to have finished with the previous frame's output.
                    auto n = star::command_order::GetPassInfo{edge.consumer};
                    cmdBus.submit(n);
                    const auto &nr = n.getReply().get();
                    waitInfo.emplace_back(vk::SemaphoreSubmitInfo()
                                              .setSemaphore(nr.signaledSemaphore)
                                              .setValue(nr.currentSignalValue)
                                              .setStageMask(vk::PipelineStageFlagBits2::eAllCommands));
                }
                else if (edge.consumer == commandBuffer)
                {
                    // We are the consumer: wait for the producer to have written this frame's output.
                    auto n = star::command_order::GetPassInfo{edge.producer};
                    cmdBus.submit(n);
                    const auto &nr = n.getReply().get();
                    waitInfo.emplace_back(vk::SemaphoreSubmitInfo()
                                              .setSemaphore(nr.signaledSemaphore)
                                              .setValue(nr.toSignalValue)
                                              .setStageMask(vk::PipelineStageFlagBits2::eAllCommands));
                }
            }
        }
    }

    // Data/transfer semaphores handed in by the command-buffer manager.
    for (const auto &wait : dataWaits)
    {
        waitInfo.emplace_back(vk::SemaphoreSubmitInfo()
                                  .setSemaphore(wait.semaphore)
                                  .setValue(wait.signaledValue.value_or(0))
                                  .setStageMask(vk::PipelineStageFlagBits2::eAllCommands));
    }

    vk::Semaphore binarySemaphore{buffer.getCompleteSemaphores()[ii]};

    std::vector<vk::SemaphoreSubmitInfo> signalInfo;
    signalInfo.emplace_back(vk::SemaphoreSubmitInfo()
                                .setSemaphore(mySemaphore)
                                .setValue(mySignalValue)
                                .setStageMask(vk::PipelineStageFlagBits2::eAllCommands));
    if (signalBinaryCompletion)
    {
        signalInfo.emplace_back(vk::SemaphoreSubmitInfo()
                                    .setSemaphore(binarySemaphore)
                                    .setStageMask(vk::PipelineStageFlagBits2::eAllCommands));
    }

    const auto cbInfo = vk::CommandBufferSubmitInfo().setCommandBuffer(
        buffer.buffer(frameTracker.getCurrent().getFrameInFlightIndex()));

    queue.getVulkanQueue().submit2(
        vk::SubmitInfo2().setWaitSemaphoreInfos(waitInfo).setCommandBufferInfos(cbInfo).setSignalSemaphoreInfos(
            signalInfo));

    return signalBinaryCompletion ? binarySemaphore : vk::Semaphore{};
}
} // namespace star::core::renderer
