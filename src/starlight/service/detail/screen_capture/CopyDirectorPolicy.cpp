#include "service/detail/screen_capture/CopyDirectorPolicy.hpp"

#include <star_common/helper/CastHelpers.hpp>
#include "core/device/managers/Semaphore.hpp"
#include "core/device/system/event/ManagerRequest.hpp"
#include "core/device/system/event/StartOfNextFrame.hpp"
#include "logging/LoggingFactory.hpp"

#include <star_common/HandleTypeRegistry.hpp>

namespace star::service::detail::screen_capture
{

static void RecordImageBarrierPostBlitPreTransfer(vk::CommandBuffer cmd, vk::Image targetImage)
{
    const auto range = vk::ImageSubresourceRange()
                           .setAspectMask(vk::ImageAspectFlagBits::eColor)
                           .setBaseMipLevel(0)
                           .setLevelCount(1)
                           .setBaseArrayLayer(0)
                           .setLayerCount(1);

    auto barrier = vk::ImageMemoryBarrier2()
                       .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
                       .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
                       .setSubresourceRange(range)
                       .setImage(targetImage)
                       .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                       .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                       .setSrcStageMask(vk::PipelineStageFlagBits2::eBlit)
                       .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
                       .setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
                       .setDstAccessMask(vk::AccessFlagBits2::eTransferRead);

    cmd.pipelineBarrier2(vk::DependencyInfo().setImageMemoryBarrierCount(1).setPImageMemoryBarriers(&barrier));
}

static void RecordBlitImage(vk::CommandBuffer cmd, vk::Image srcImage, vk::Image dstImage,
                            const vk::Extent3D &imageExtent, const vk::Filter &filter)
{
    auto region = vk::ImageBlit2()
                      .setSrcOffsets({vk::Offset3D{0, 0, 0}})
                      .setSrcSubresource(vk::ImageSubresourceLayers()
                                             .setBaseArrayLayer(0)
                                             .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                             .setLayerCount(1)
                                             .setMipLevel(0))
                      .setDstOffsets({vk::Offset3D{0, 0, 0}})
                      .setDstSubresource(vk::ImageSubresourceLayers()
                                             .setBaseArrayLayer(0)
                                             .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                             .setLayerCount(1)
                                             .setMipLevel(0));

    cmd.blitImage2(vk::BlitImageInfo2()
                       .setSrcImage(srcImage)
                       .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
                       .setDstImage(dstImage)
                       .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
                       .setFilter(filter)
                       .setRegions(region));
}

void DefaultCopyPolicy::init(DeviceInfo &deviceInfo)
{
    m_deviceInfo = &deviceInfo;
    createSemaphores(*m_deviceInfo->eventBus, m_deviceInfo->flightTracker->getSetup().getNumFramesInFlight());
}

GPUSynchronizationInfo DefaultCopyPolicy::triggerSubmission(CopyPlan &copyPlan, const uint8_t &frameInFlightIndex)
{
    prepareInProgressResources(copyPlan, frameInFlightIndex);

    assert(m_deviceInfo->commandManager != nullptr);
    m_copyCmds.trigger(*m_deviceInfo->commandManager);

    // m_deviceInfo->commandManager->submitDynamicBuffer(m_commandBuffer);

    return GPUSynchronizationInfo{.semaphore = *m_doneSemaphoresRaw[frameInFlightIndex],
                                  .signalValue =
                                      m_deviceInfo->flightTracker->getCurrent().getFramesInFlightTracking().getNumOfTimesFrameProcessed(frameInFlightIndex)};
}

void DefaultCopyPolicy::prepareInProgressResources(CopyPlan &copyPlan, const uint8_t &frameInFlightIndex) noexcept
{
    m_inUseResources->path = copyPlan.path;
    m_inUseResources->targetImage = copyPlan.calleeDependencies->targetTexture.getVulkanImage();
    m_inUseResources->buffer = copyPlan.resources.bufferInfo.hostVisibleBuffer.getVulkanBuffer();
    m_inUseResources->targetImageLayout = copyPlan.calleeDependencies->targetTexture.getImageLayout();
    m_inUseResources->targetImageExtent = copyPlan.calleeDependencies->targetTexture.getBaseExtent();
    m_inUseResources->blitFilter = copyPlan.blitFilter;

    if (copyPlan.resources.blitTargetTexture.has_value())
    {
        m_inUseResources->targetBlitImage = copyPlan.resources.blitTargetTexture.value();
    }
    if (copyPlan.calleeDependencies->targetTextureReadySemaphore.has_value())
    {
        m_inUseResources->targetTextureReadySemaphore =
            &m_deviceInfo->semaphoreManager->get(copyPlan.calleeDependencies->targetTextureReadySemaphore.value())
                 ->semaphore;
    }

    m_inUseResources->numTimesFrameProcessed = m_deviceInfo->flightTracker->getCurrent().getFramesInFlightTracking().getNumOfTimesFrameProcessed(frameInFlightIndex);
    m_inUseResources->semaphoreForCopyDone = m_binarySignalSemaphoresRaw[frameInFlightIndex];
    m_inUseResources->timelineSemaphoreForCopyDone = m_doneSemaphoresRaw[frameInFlightIndex];
    m_inUseResources->queueToUse = m_deviceInfo->device->getDefaultQueue(Queue_Type::Ttransfer).getVulkanQueue();
}

void DefaultCopyPolicy::registerWithCommandBufferManager()
{
    m_copyCmds.init(*m_deviceInfo->device, *m_deviceInfo->commandManager);
    m_blitCmds.init(*m_deviceInfo->device, *m_deviceInfo->commandManager);
}

void DefaultCopyPolicy::createSemaphores(star::common::EventBus &eventBus, const uint8_t &numFramesInFlight)
{
    m_doneSemaphoreHandles.resize(numFramesInFlight);
    m_doneSemaphoresRaw.resize(numFramesInFlight);
    m_binarySignalSemaphoresHandles.resize(numFramesInFlight);
    m_binarySignalSemaphoresRaw.resize(numFramesInFlight);

    for (uint8_t i = 0; i < numFramesInFlight; i++)
    {
        {
            void *r = nullptr;
            eventBus.emit(core::device::system::event::ManagerRequest{
                star::common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                    core::device::manager::GetSemaphoreEventTypeName),
                core::device::manager::SemaphoreRequest{true}, m_doneSemaphoreHandles[i], &r});

            assert(r != nullptr && m_doneSemaphoreHandles[i].isInitialized() && "Emit did not provide a result");
            m_doneSemaphoresRaw[i] = &static_cast<core::device::manager::SemaphoreRecord *>(r)->semaphore;
        }

        {
            void *r = nullptr;
            eventBus.emit(core::device::system::event::ManagerRequest{
                star::common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                    core::device::manager::GetSemaphoreEventTypeName),
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
    uint64_t signaledSemaphoreValue = m_deviceInfo->flightTracker->getCurrent().getFramesInFlightTracking().getNumOfTimesFrameProcessed(frameInFlightIndex);

    m_deviceInfo->eventBus->subscribe(
        star::common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
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
        .oneTimeWaitSemaphoreInfo.insert(m_doneSemaphoreHandles[targetFrameInFlightIndex],
                                         *m_doneSemaphoresRaw[targetFrameInFlightIndex],
                                         vk::PipelineStageFlagBits::eColorAttachmentOutput, signaledSemaphoreValue);
    keepAlive = false;
}

StarTextures::Texture DefaultCopyPolicy::createBlitTargetTexture(const vk::Extent2D &extent) const
{
    return StarTextures::Texture::Builder(m_deviceInfo->device->getVulkanDevice(),
                                          m_deviceInfo->device->getAllocator().get())
        .build();
}

} // namespace star::service::detail::screen_capture