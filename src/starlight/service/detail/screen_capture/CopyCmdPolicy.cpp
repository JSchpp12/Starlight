#include "service/detail/screen_capture/CopyCmdPolicy.hpp"

#include "core/Exceptions.hpp"
#include "logging/LoggingFactory.hpp"

#include <star_common/helper/CastHelpers.hpp>

#include <cassert>

namespace star::service::detail::screen_capture
{
Handle CopyCmdPolicy::registerWithManager(core::device::StarDevice &device,
                                          core::device::manager::ManagerCommandBuffer &manCmdBuf)
{
    return manCmdBuf.submit(
        device, 0,
        core::device::manager::ManagerCommandBuffer::Request{
            .recordBufferCallback = std::bind(&CopyCmdPolicy::recordCommandBuffer, this, std::placeholders::_1,
                                              std::placeholders::_2, std::placeholders::_3),
            .order = Command_Buffer_Order::end_of_frame,
            .orderIndex = Command_Buffer_Order_Index::first,
            .type = Queue_Type::Ttransfer,
            .waitStage = vk::PipelineStageFlagBits::eAllCommands,
            .willBeSubmittedEachFrame = false,
            .recordOnce = false,
            .overrideBufferSubmissionCallback = std::bind(
                &CopyCmdPolicy::submitBuffer, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7)});
}

static vk::BufferMemoryBarrier2 GetBarrierPrepForCPURead(vk::Buffer buffer) noexcept
{
    return vk::BufferMemoryBarrier2()
        .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
        .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eHost)
        .setDstAccessMask(vk::AccessFlagBits2::eHostRead)
        .setBuffer(buffer)
        .setOffset(0)
        .setSize(VK_WHOLE_SIZE);
}

void CopyCmdPolicy::addMemoryDependenciesToCleanupFromCopy(vk::CommandBuffer &commandBuffer)
{
    // auto imageBarriers = getImageBarriersForCleanup();

    const vk::BufferMemoryBarrier2 buffBarrier[1]{GetBarrierPrepForCPURead(m_inUseInfo->buffer)};

    commandBuffer.pipelineBarrier2(vk::DependencyInfo().setBufferMemoryBarriers(buffBarrier));
}

void CopyCmdPolicy::init(core::device::StarDevice &device)
{
    m_device = &device;
}

std::vector<vk::ImageMemoryBarrier2> CopyCmdPolicy::getImageBarriersForPrep() const
{
    const auto range = vk::ImageSubresourceRange()
                           .setAspectMask(vk::ImageAspectFlagBits::eColor)
                           .setBaseMipLevel(0)
                           .setLevelCount(vk::RemainingMipLevels)
                           .setBaseArrayLayer(0)
                           .setLayerCount(vk::RemainingArrayLayers);

    auto barriers = std::vector<vk::ImageMemoryBarrier2>(1);
    if (m_inUseInfo->targetImageLayout == vk::ImageLayout::ePresentSrcKHR)
    {
        barriers[0]
            .setOldLayout(vk::ImageLayout::ePresentSrcKHR)
            .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setSubresourceRange(range)
            .setImage(m_inUseInfo->targetImage)
            .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
            .setSrcAccessMask(vk::AccessFlagBits2::eNone)
            .setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
            .setDstAccessMask(vk::AccessFlagBits2::eTransferRead);
    }
    else
    {
        barriers[0]
            .setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setSubresourceRange(range)
            .setImage(m_inUseInfo->targetImage)
            .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
            .setSrcAccessMask(vk::AccessFlagBits2::eNone)
            .setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
            .setDstAccessMask(vk::AccessFlagBits2::eTransferRead);
    }

    return barriers;
}

std::vector<vk::ImageMemoryBarrier2> CopyCmdPolicy::getImageBarriersForCleanup() const
{
    return {vk::ImageMemoryBarrier2()
                .setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
                .setNewLayout(m_inUseInfo->targetImageLayout)
                .setSubresourceRange(vk::ImageSubresourceRange()
                                         .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                         .setBaseMipLevel(0)
                                         .setLevelCount(1)
                                         .setBaseArrayLayer(0)
                                         .setLayerCount(1))
                .setImage(m_inUseInfo->targetImage)
                .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
                .setSrcAccessMask(vk::AccessFlagBits2::eTransferRead)
                .setDstStageMask(vk::PipelineStageFlagBits2::eBottomOfPipe)
                .setDstAccessMask(vk::AccessFlagBits2::eNone)};
}

vk::Semaphore CopyCmdPolicy::submitBuffer(StarCommandBuffer &buffer, const star::common::FrameTracker &frameTracker,
                                          std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                                          std::vector<vk::Semaphore> dataSemaphores,
                                          std::vector<vk::PipelineStageFlags> dataWaitPoints,
                                          std::vector<std::optional<uint64_t>> previousSignaledValues, StarQueue &queue)
{
    assert(m_inUseInfo->timelineSemaphoreForCopyDone.semaphore &&
           m_inUseInfo->timelineSemaphoreForCopyDone.signaledValue && m_inUseInfo->binarySemaphoreForCopyDone &&
           "Timeline semaphore should have been set previously");

    const uint64_t signalSemaphoreValues[1]{m_inUseInfo->timelineSemaphoreForCopyDone.valueToSignal};
    const vk::Semaphore signalTimelineSemaphores[1]{*m_inUseInfo->timelineSemaphoreForCopyDone.semaphore};
    *m_inUseInfo->timelineSemaphoreForCopyDone.signaledValue = m_inUseInfo->timelineSemaphoreForCopyDone.valueToSignal;
    std::vector<vk::SemaphoreSubmitInfo> signalInfo(1);
    for (int i{0}; i < 1; i++)
    {
        signalInfo[i] = vk::SemaphoreSubmitInfo()
                            .setSemaphore(signalTimelineSemaphores[i])
                            .setStageMask(vk::PipelineStageFlagBits2::eAllCommands)
                            .setValue(signalSemaphoreValues[i]);
    }

    std::optional<std::vector<uint64_t>> waitValues = std::nullopt;
    std::vector<vk::SemaphoreSubmitInfo> waitSemaphores = std::vector<vk::SemaphoreSubmitInfo>(1);
    if (previousSignaledValues.size() > 0)
    {
        // some callee has provided manual wait information. Respect that first
        waitSemaphores[0] =
            vk::SemaphoreSubmitInfo()
                .setSemaphore(dataSemaphores[0])
                .setStageMask(vk::PipelineStageFlagBits2::eTransfer)
                .setValue(previousSignaledValues[0].has_value() ? previousSignaledValues[0].value() : 0);
    }
    else if (m_inUseInfo->targetTextureReadySemaphore != nullptr)
    {
        waitSemaphores[0] = vk::SemaphoreSubmitInfo()
                                .setSemaphore(*m_inUseInfo->targetTextureReadySemaphore)
                                .setStageMask(vk::PipelineStageFlagBits2::eAllCommands)
                                .setValue(0);
    }
    else
    {
        assert(previousCommandBufferSemaphores != nullptr);
        waitSemaphores[0] = vk::SemaphoreSubmitInfo()
                                .setSemaphore(previousCommandBufferSemaphores->at(0))
                                .setStageMask(vk::PipelineStageFlagBits2::eAllCommands)
                                .setValue(0);
    }

    const auto subInfo = vk::CommandBufferSubmitInfo().setCommandBuffer(
        buffer.buffer(frameTracker.getCurrent().getFrameInFlightIndex()));

    const auto submitInfo = vk::SubmitInfo2()
                                .setWaitSemaphoreInfos(waitSemaphores)
                                .setCommandBufferInfos(subInfo)
                                .setSignalSemaphoreInfos(signalInfo);

    assert(m_inUseInfo->queueToUse != nullptr);

    m_inUseInfo->queueToUse.submit2(submitInfo);

    return vk::Semaphore();
}

void CopyCmdPolicy::recordCopyImageToBuffer(vk::CommandBuffer &commandBuffer, vk::Image targetSrcImage) const
{
    const auto copy = vk::BufferImageCopy2()
                          .setImageExtent(m_inUseInfo->targetImageExtent)
                          .setImageSubresource(vk::ImageSubresourceLayers()
                                                   .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                   .setBaseArrayLayer(0)
                                                   .setLayerCount(1)
                                                   .setMipLevel(0));
    commandBuffer.copyImageToBuffer2(vk::CopyImageToBufferInfo2()
                                         .setSrcImage(targetSrcImage)
                                         .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
                                         .setDstBuffer(m_inUseInfo->buffer)
                                         .setRegions(copy));
}

void CopyCmdPolicy::waitForSemaphoreIfNecessary(const star::common::FrameTracker &frameTracker) const
{
    const uint64_t &frameCount = frameTracker.getCurrent().getNumTimesFrameProcessed();
    if (frameCount == m_inUseInfo->timelineSemaphoreForCopyDone.signaledValue->value())
    {
        assert(m_inUseInfo->timelineSemaphoreForCopyDone.semaphore && "Semaphore was not assigned");
        assert(m_device != nullptr && "Init() was never called");

        auto result = m_device->getVulkanDevice().waitSemaphores(
            vk::SemaphoreWaitInfo()
                .setValues(frameCount)
                .setSemaphores(*m_inUseInfo->timelineSemaphoreForCopyDone.semaphore),
            UINT64_MAX);

        if (result != vk::Result::eSuccess)
        {
            STAR_THROW("Failed to wait for timeline semaphores");
        }
    }
}

void CopyCmdPolicy::recordCommandBuffer(StarCommandBuffer &commandBuffer,
                                        const star::common::FrameTracker &frameTracker, const uint64_t &frameIndex)
{
    waitForSemaphoreIfNecessary(frameTracker);

    commandBuffer.begin(frameTracker.getCurrent().getFrameInFlightIndex());
    addMemoryDependenciesToPrepForCopy(commandBuffer.buffer(frameTracker.getCurrent().getFrameInFlightIndex()));
    recordCopyImageToBuffer(commandBuffer.buffer(frameTracker.getCurrent().getFrameInFlightIndex()),
                            m_inUseInfo->targetImage);
    addMemoryDependenciesToCleanupFromCopy(commandBuffer.buffer(frameTracker.getCurrent().getFrameInFlightIndex()));

    commandBuffer.buffer(frameTracker.getCurrent().getFrameInFlightIndex()).end();
}

void CopyCmdPolicy::recordCopyCommands(vk::CommandBuffer &commandBuffer) const
{
    recordCopyImageToBuffer(commandBuffer, m_inUseInfo->targetImage);
}

void CopyCmdPolicy::addMemoryDependenciesToPrepForCopy(vk::CommandBuffer &commandBuffer)
{
    // assuming that the target image is not in the proper layout for transfer SRC
    auto imageBarriers = getImageBarriersForPrep();
    uint32_t numImageBarriers;
    star::common::casts::SafeCast<size_t, uint32_t>(imageBarriers.size(), numImageBarriers);

    commandBuffer.pipelineBarrier2(vk::DependencyInfo()
                                       .setImageMemoryBarrierCount(numImageBarriers)
                                       .setPImageMemoryBarriers(imageBarriers.data()));
}

} // namespace star::service::detail::screen_capture