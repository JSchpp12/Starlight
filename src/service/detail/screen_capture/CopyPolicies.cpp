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
    m_doneSemaphores = createSemaphores(*m_deviceInfo->eventBus, m_deviceInfo->numFramesInFlight);
}

void DefaultCopyPolicy::triggerSubmission(CalleeRenderDependencies &targetDeps, const uint8_t &frameInFlightIndex)
{
    m_targetDeps = &targetDeps;

    m_deviceInfo->commandManager->submitDynamicBuffer(m_commandBuffer);

    registerListenerForNextFrameStart(targetDeps, frameInFlightIndex);
}

void DefaultCopyPolicy::recordCommandBuffer(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                            const uint64_t &frameIndex)
{
    core::logging::log(boost::log::trivial::info, "Start record");

    addMemoryDependencies(commandBuffer);
    recordCopyCommands(commandBuffer);
}

void DefaultCopyPolicy::recordCopyCommands(vk::CommandBuffer &commandBuffer) const
{
    const vk::Extent3D imageExtent = m_targetDeps->targetTexture.getBaseExtent();

    commandBuffer.copyImageToBuffer2(
        vk::CopyImageToBufferInfo2().setSrcImage(m_targetDeps->targetTexture.getVulkanImage()).setSrcImageLayout(vk::ImageLayout::eTransferDstOptimal)
        .setDstBuffer(m_targetDeps->hostVisibleBuffers.at(0).getVulkanBuffer())
        .setRegions({vk::BufferImageCopy2().setImageExtent(imageExtent)})
    );
}

void DefaultCopyPolicy::addMemoryDependencies(vk::CommandBuffer &commandBuffer) const
{
    // assuming that the target image is not in the proper layout for transfer SRC
    auto imageBarriers = getImageBarriersForThisFrame();
    uint32_t numImageBarriers;
    CastHelpers::SafeCast<size_t, uint32_t>(imageBarriers.size(), numImageBarriers);

    commandBuffer.pipelineBarrier2(vk::DependencyInfo()
                                       .setImageMemoryBarrierCount(numImageBarriers)
                                       .setPImageMemoryBarriers(imageBarriers.data()));
}

std::vector<vk::ImageMemoryBarrier2> DefaultCopyPolicy::getImageBarriersForThisFrame() const
{
    assert(m_targetDeps != nullptr);
    const auto range = vk::ImageSubresourceRange()
                           .setAspectMask(vk::ImageAspectFlagBits::eColor)
                           .setBaseMipLevel(0)
                           .setLevelCount(1)
                           .setBaseArrayLayer(0)
                           .setLayerCount(1);

    auto barriers = std::vector<vk::ImageMemoryBarrier2>(2);
    if (m_targetDeps->targetTexture.getImageLayout() == vk::ImageLayout::ePresentSrcKHR)
    {
        barriers[0] = vk::ImageMemoryBarrier2()
                          .setOldLayout(vk::ImageLayout::ePresentSrcKHR)
                          .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
                          .setSubresourceRange(range)
                          .setImage(m_targetDeps->targetTexture.getVulkanImage())
                          .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                          .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                          .setSrcStageMask(vk::PipelineStageFlagBits2::eAllCommands)
                          .setSrcAccessMask(vk::AccessFlagBits2::eMemoryRead)
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

    barriers[1] = vk::ImageMemoryBarrier2()
                      .setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
                      .setNewLayout(m_targetDeps->targetTexture.getImageLayout())
                      .setSubresourceRange(range)
                      .setImage(m_targetDeps->targetTexture.getVulkanImage())
                      .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                      .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                      .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
                      .setSrcAccessMask(vk::AccessFlagBits2::eTransferRead)
                      .setDstStageMask(vk::PipelineStageFlagBits2::eBottomOfPipe)
                      .setDstAccessMask(vk::AccessFlagBits2::eNone);

    return barriers;
}

void DefaultCopyPolicy::registerWithCommandBufferManager()
{
    m_commandBuffer = m_deviceInfo->commandManager->submit(
        *m_deviceInfo->device, *m_deviceInfo->currentFrameCounter,
        core::device::manager::ManagerCommandBuffer::Request{
            .recordBufferCallback = std::bind(&DefaultCopyPolicy::recordCommandBuffer, this, std::placeholders::_1,
                                              std::placeholders::_2, std::placeholders::_3),
            .order = star::Command_Buffer_Order::after_presentation,
            .orderIndex = Command_Buffer_Order_Index::first,
            .type = Queue_Type::Ttransfer,
            .waitStage = vk::PipelineStageFlagBits::eTransfer,
            .recordOnce = false,
            .willBeSubmittedEachFrame = false,
            .overrideBufferSubmissionCallback =
                std::bind(&DefaultCopyPolicy::submitBuffer, this, std::placeholders::_1, std::placeholders::_2,
                          std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)});
}

vk::Semaphore DefaultCopyPolicy::submitBuffer(StarCommandBuffer &buffer, const int &frameIndexToBeDrawn,
                                              std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                                              std::vector<vk::Semaphore> dataSemaphores,
                                              std::vector<vk::PipelineStageFlags> dataWaitPoints)
{
    buffer.buffer();
    

    return vk::Semaphore();
}

std::vector<Handle> DefaultCopyPolicy::createSemaphores(core::device::system::EventBus &eventBus,
                                                        const uint8_t &numFramesInFlight)
{
    auto result = std::vector<Handle>(numFramesInFlight);

    for (uint8_t i = 0; i < numFramesInFlight; i++)
    {
        eventBus.emit(
            core::device::system::event::ManagerRequest{common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                                                            core::device::manager::SemaphoreEventTypeName()),
                                                        core::device::manager::SemaphoreRequest{true}, result[i]});

        assert(result[i].isInitialized() && "Emit did not provide a result");
    }

    return result;
}

void DefaultCopyPolicy::registerListenerForNextFrameStart(CalleeRenderDependencies &deps,
                                                          const uint8_t &frameInFlightIndex)
{
    Handle calleeHandle = deps.commandBufferContainingTarget;
    uint8_t targetFrameInFlightIndex = frameInFlightIndex;

    m_deviceInfo->eventBus->subscribe(
        common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
            core::device::system::event::StartOfNextFrameName()),
        {[this, calleeHandle, targetFrameInFlightIndex](const star::common::IEvent &e, bool &keepAlive) {
             this->startOfFrameEventCallback(calleeHandle, targetFrameInFlightIndex, e, keepAlive);
         },
         [this]() -> Handle * { return &this->m_startOfFrameListener; },
         [this](const Handle &noLongerNeededHandle) { this->m_startOfFrameListener = Handle(); }});
}

void DefaultCopyPolicy::startOfFrameEventCallback(const Handle &calleeCommandBuffer,
                                                  const uint8_t &targetFrameInFlightIndex,
                                                  const star::common::IEvent &e, bool &keepAlive)
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
        .oneTimeWaitSemaphoreInfo.insert(m_doneSemaphores.front(),
                                         m_deviceInfo->semaphoreManager->get(m_doneSemaphores.front())->semaphore,
                                         vk::PipelineStageFlagBits::eColorAttachmentOutput);
    keepAlive = false;
}

} // namespace star::service::detail::screen_capture