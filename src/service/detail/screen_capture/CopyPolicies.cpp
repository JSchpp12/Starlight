#include "service/detail/screen_capture/CopyPolicies.hpp"

#include "core/device/managers/Semaphore.hpp"
#include "core/device/system/event/ManagerRequest.hpp"
#include "core/device/system/event/StartOfNextFrame.hpp"
#include "logging/LoggingFactory.hpp"

#include "CastHelpers.hpp"

namespace star::service::detail::screen_capture
{
void DefaultCopyPolicy::init(DeviceInfo &deviceInfo)
{
    m_deviceInfo = &deviceInfo;
    createSemaphores(*m_deviceInfo->eventBus, m_deviceInfo->numFramesInFlight);
}

SynchronizationInfo DefaultCopyPolicy::triggerSubmission(CalleeRenderDependencies &targetDeps,
                                                         const uint8_t &frameInFlightIndex)
{
    m_targetDeps = &targetDeps;

    m_deviceInfo->commandManager->submitDynamicBuffer(m_commandBuffer);

    registerListenerForNextFrameStart(targetDeps, frameInFlightIndex);

    return SynchronizationInfo{.semaphore = *m_doneSemaphoresRaw[frameInFlightIndex],
                               .signalValue =
                                   m_deviceInfo->frameTracker->getNumOfTimesFrameProcessed(frameInFlightIndex)};
}

void DefaultCopyPolicy::recordCommandBuffer(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                            const uint64_t &frameIndex)
{
    core::logging::log(boost::log::trivial::info, "Start record");

    addMemoryDependenciesToPrepForCopy(commandBuffer);
    recordCopyCommands(commandBuffer);
    addMemoryDependenciesToCleanupFromCopy(commandBuffer);
}

void DefaultCopyPolicy::recordCopyCommands(vk::CommandBuffer &commandBuffer) const
{
    const vk::Extent3D imageExtent = m_targetDeps->targetTexture.getBaseExtent();

    commandBuffer.copyImageToBuffer2(
        vk::CopyImageToBufferInfo2()
            .setSrcImage(m_targetDeps->targetTexture.getVulkanImage())
            .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setDstBuffer(m_targetDeps->hostVisibleBuffers.at(0).getVulkanBuffer())
            .setRegions({vk::BufferImageCopy2()
                             .setImageExtent(imageExtent)
                             .setImageSubresource(vk::ImageSubresourceLayers()
                                                      .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                      .setBaseArrayLayer(0)
                                                      .setLayerCount(1)
                                                      .setMipLevel(0))}));
}

void DefaultCopyPolicy::addMemoryDependenciesToPrepForCopy(vk::CommandBuffer &commandBuffer) const
{
    // assuming that the target image is not in the proper layout for transfer SRC
    auto imageBarriers = getImageBarriersForPrep();
    uint32_t numImageBarriers;
    CastHelpers::SafeCast<size_t, uint32_t>(imageBarriers.size(), numImageBarriers);

    commandBuffer.pipelineBarrier2(vk::DependencyInfo()
                                       .setImageMemoryBarrierCount(numImageBarriers)
                                       .setPImageMemoryBarriers(imageBarriers.data()));
}

void DefaultCopyPolicy::addMemoryDependenciesToCleanupFromCopy(vk::CommandBuffer &commandBuffer) const
{
    auto imageBarriers = getImageBarriersForCleanup();
    uint32_t numImageBarriers;
    CastHelpers::SafeCast<size_t, uint32_t>(imageBarriers.size(), numImageBarriers);

    commandBuffer.pipelineBarrier2(vk::DependencyInfo()
                                       .setImageMemoryBarrierCount(numImageBarriers)
                                       .setPImageMemoryBarriers(imageBarriers.data()));
}

std::vector<vk::ImageMemoryBarrier2> DefaultCopyPolicy::getImageBarriersForPrep() const
{
    assert(m_targetDeps != nullptr);
    const auto range = vk::ImageSubresourceRange()
                           .setAspectMask(vk::ImageAspectFlagBits::eColor)
                           .setBaseMipLevel(0)
                           .setLevelCount(1)
                           .setBaseArrayLayer(0)
                           .setLayerCount(1);

    auto barriers = std::vector<vk::ImageMemoryBarrier2>(1);
    if (m_targetDeps->targetTexture.getImageLayout() == vk::ImageLayout::ePresentSrcKHR)
    {
        barriers[0] = vk::ImageMemoryBarrier2()
                          .setOldLayout(vk::ImageLayout::ePresentSrcKHR)
                          .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
                          .setSubresourceRange(range)
                          .setImage(m_targetDeps->targetTexture.getVulkanImage())
                          .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                          .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                          .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
                          .setSrcAccessMask(vk::AccessFlagBits2::eNone)
                          .setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
                          .setDstAccessMask(vk::AccessFlagBits2::eTransferRead);
    }
    else
    {
        barriers[0] = vk::ImageMemoryBarrier2()
                          .setOldLayout(m_targetDeps->targetTexture.getImageLayout())
                          .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
                          .setSubresourceRange(range)
                          .setImage(m_targetDeps->targetTexture.getVulkanImage())
                          .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                          .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                          .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
                          .setSrcAccessMask(vk::AccessFlagBits2::eNone)
                          .setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
                          .setDstAccessMask(vk::AccessFlagBits2::eTransferRead);
    }

    return barriers;
}

std::vector<vk::ImageMemoryBarrier2> DefaultCopyPolicy::getImageBarriersForCleanup() const
{
    const auto range = vk::ImageSubresourceRange()
                           .setAspectMask(vk::ImageAspectFlagBits::eColor)
                           .setBaseMipLevel(0)
                           .setLevelCount(1)
                           .setBaseArrayLayer(0)
                           .setLayerCount(1);
    return {vk::ImageMemoryBarrier2()
                .setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
                .setNewLayout(m_targetDeps->targetTexture.getImageLayout())
                .setSubresourceRange(range)
                .setImage(m_targetDeps->targetTexture.getVulkanImage())
                .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
                .setSrcAccessMask(vk::AccessFlagBits2::eTransferRead)
                .setDstStageMask(vk::PipelineStageFlagBits2::eBottomOfPipe)
                .setDstAccessMask(vk::AccessFlagBits2::eNone)};
}

void DefaultCopyPolicy::registerWithCommandBufferManager()
{
    m_commandBuffer = m_deviceInfo->commandManager->submit(
        *m_deviceInfo->device, *m_deviceInfo->currentFrameCounter,
        core::device::manager::ManagerCommandBuffer::Request{
            .recordBufferCallback = std::bind(&DefaultCopyPolicy::recordCommandBuffer, this, std::placeholders::_1,
                                              std::placeholders::_2, std::placeholders::_3),
            .order = star::Command_Buffer_Order::end_of_frame,
            .orderIndex = Command_Buffer_Order_Index::first,
            .type = Queue_Type::Ttransfer,
            .waitStage = vk::PipelineStageFlagBits::eTransfer,
            .willBeSubmittedEachFrame = false,
            .recordOnce = false,
            .overrideBufferSubmissionCallback =
                std::bind(&DefaultCopyPolicy::submitBuffer, this, std::placeholders::_1, std::placeholders::_2,
                          std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6)});
}

vk::Semaphore DefaultCopyPolicy::submitBuffer(StarCommandBuffer &buffer, const int &frameIndexToBeDrawn,
                                              std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                                              std::vector<vk::Semaphore> dataSemaphores,
                                              std::vector<vk::PipelineStageFlags> dataWaitPoints,
                                              std::vector<std::optional<uint64_t>> previousSignaledValues)
{
    auto &queue = m_deviceInfo->device->getDefaultQueue(Queue_Type::Ttransfer);
    const std::vector<uint64_t> signalSemaphoreValues{
        m_deviceInfo->frameTracker->getNumOfTimesFrameProcessed(frameIndexToBeDrawn), 0};
    const std::vector<vk::Semaphore> signaltimelineSemaphores{*m_doneSemaphoresRaw[frameIndexToBeDrawn],
                                                              *m_binarySignalSemaphoresRaw[frameIndexToBeDrawn]};
    uint32_t semaphoreCount; 
    CastHelpers::SafeCast<size_t, uint32_t>(signalSemaphoreValues.size(), semaphoreCount); 

    auto timelineSubmitInfo = vk::TimelineSemaphoreSubmitInfo()
                                  .setPSignalSemaphoreValues(signalSemaphoreValues.data())
                                  .setSignalSemaphoreValueCount(semaphoreCount);

    std::vector<vk::Semaphore> waitSemaphores{
        m_deviceInfo->semaphoreManager->get(m_targetDeps->targetTextureReadySemaphore)->semaphore};
    std::vector<vk::PipelineStageFlags> waitPoints{vk::PipelineStageFlagBits::eTransfer};

    auto submitInfo = vk::SubmitInfo()
                          .setCommandBufferCount(1)
                          .setCommandBuffers(buffer.buffer(frameIndexToBeDrawn))
                          .setWaitSemaphoreCount(1)
                          .setPWaitSemaphores(waitSemaphores.data())
                          .setPWaitDstStageMask(waitPoints.data())
                          .setPSignalSemaphores(signaltimelineSemaphores.data())
                          .setSignalSemaphoreCount(semaphoreCount)
                          .setPNext(&timelineSubmitInfo);

    m_deviceInfo->device->getDefaultQueue(star::Queue_Type::Ttransfer).getVulkanQueue().submit({submitInfo});

    return *m_binarySignalSemaphoresRaw[frameIndexToBeDrawn];
}

void DefaultCopyPolicy::createSemaphores(core::device::system::EventBus &eventBus, const uint8_t &numFramesInFlight)
{
    m_doneSemaphoreHandles.resize(numFramesInFlight);
    m_doneSemaphoresRaw.resize(numFramesInFlight);
    m_binarySignalSemaphoresHandles.resize(numFramesInFlight);
    m_binarySignalSemaphoresRaw.resize(numFramesInFlight);

    for (uint8_t i = 0; i < numFramesInFlight; i++)
    {
        {
            void *r = nullptr;
            // void** ptrToResult
            eventBus.emit(core::device::system::event::ManagerRequest{
                common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                    core::device::manager::SemaphoreEventTypeName()),
                core::device::manager::SemaphoreRequest{true}, m_doneSemaphoreHandles[i], &r});

            assert(r != nullptr && m_doneSemaphoreHandles[i].isInitialized() && "Emit did not provide a result");
            m_doneSemaphoresRaw[i] = &static_cast<core::device::manager::SemaphoreRecord *>(r)->semaphore;
        }

        {
            void *r = nullptr;
            eventBus.emit(core::device::system::event::ManagerRequest{
                common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                    core::device::manager::SemaphoreEventTypeName()),
                core::device::manager::SemaphoreRequest{false}, m_binarySignalSemaphoresHandles[i], &r});

            m_binarySignalSemaphoresRaw[i] = &static_cast<core::device::manager::SemaphoreRecord *>(r)->semaphore;
        }
    }
}

void DefaultCopyPolicy::registerListenerForNextFrameStart(CalleeRenderDependencies &deps,
                                                          const uint8_t &frameInFlightIndex)
{
    Handle calleeHandle = deps.commandBufferContainingTarget;
    uint8_t targetFrameInFlightIndex = frameInFlightIndex;
    uint64_t signaledSemaphoreValue = m_deviceInfo->frameTracker->getNumOfTimesFrameProcessed(frameInFlightIndex);

    m_deviceInfo->eventBus->subscribe(
        common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
            core::device::system::event::StartOfNextFrameName()),
        {[this, calleeHandle, targetFrameInFlightIndex, signaledSemaphoreValue](const star::common::IEvent &e,
                                                                                bool &keepAlive) {
             this->startOfFrameEventCallback(calleeHandle, targetFrameInFlightIndex, signaledSemaphoreValue, e,
                                             keepAlive);
         },
         [this]() -> Handle * { return &this->m_startOfFrameListener; },
         [this](const Handle &noLongerNeededHandle) { this->m_startOfFrameListener = Handle(); }});
}

void DefaultCopyPolicy::startOfFrameEventCallback(const Handle &calleeCommandBuffer,
                                                  const uint8_t &targetFrameInFlightIndex,
                                                  const uint64_t &signaledSemaphoreValue, const star::common::IEvent &e,
                                                  bool &keepAlive)
{
    assert(calleeCommandBuffer.isInitialized() && "Callee information must be provided");

    const auto &startEvent = static_cast<const core::device::system::event::StartOfNextFrame &>(e);

    // check if the previous image matches with the image that WILL be used this frame in the target command buffer
    if (targetFrameInFlightIndex != startEvent.getFrameInFlightIndex())
    {
        keepAlive = true;
        return;
    }

    m_deviceInfo->commandManager->get(calleeCommandBuffer)
        .oneTimeWaitSemaphoreInfo.insert(m_doneSemaphoreHandles.front(), *m_doneSemaphoresRaw.front(),
                                         vk::PipelineStageFlagBits::eColorAttachmentOutput, signaledSemaphoreValue);
    keepAlive = false;
}

} // namespace star::service::detail::screen_capture