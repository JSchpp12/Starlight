#include "service/detail/screen_capture/CopyDirectorPolicy.hpp"

#include "core/device/managers/Semaphore.hpp"
#include "core/device/system/event/ManagerRequest.hpp"
#include "core/helper/queue/QueueHelpers.hpp"
#include "event/GetQueue.hpp"
#include "logging/LoggingFactory.hpp"

#include <star_common/helper/CastHelpers.hpp>

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

    const auto barrier = vk::ImageMemoryBarrier2()
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
    const auto region = vk::ImageBlit2()
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
    initSemaphores(m_deviceInfo->flightTracker->getSetup().getNumUniqueTargetFramesForFinalization());
}

GPUSynchronizationInfo DefaultCopyPolicy::triggerSubmission(CopyPlan &copyPlan)
{
    prepareInProgressResources(copyPlan);

    assert(m_deviceInfo->commandManager != nullptr);
    m_copyCmds.trigger(*m_deviceInfo->commandManager);

    return GPUSynchronizationInfo{.signalValue = m_inUseResources->timelineSemaphoreForCopyDone.valueToSignal,
                                  .copyCommandBuffer = m_copyCmds.getCommandBuffer(),
                                  .binarySemaphoreForMainCopyDone = *m_inUseResources->binarySemaphoreForCopyDone,

                                  .timelineSemaphoreForMainCopyCommandsDone =
                                      *m_inUseResources->timelineSemaphoreForCopyDone.semaphore};
}

void DefaultCopyPolicy::prepareInProgressResources(CopyPlan &copyPlan) noexcept
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
            &copyPlan.calleeDependencies->targetTextureReadySemaphore.value();
    }

    {
        const size_t frameInFlight =
            static_cast<size_t>(m_deviceInfo->flightTracker->getCurrent().getFrameInFlightIndex());

        if (!m_timelineInfo.handles[frameInFlight].isInitialized())
        {
            m_timelineInfo.createSemaphoreDataAtIndex(
                *m_deviceInfo->eventBus, frameInFlight,
                m_deviceInfo->flightTracker->getCurrent().getNumTimesFrameProcessed());
        }

        auto *record = m_deviceInfo->semaphoreManager->get(m_timelineInfo.handles[frameInFlight]);
        m_inUseResources->timelineSemaphoreForCopyDone.semaphore = &record->semaphore;
        m_inUseResources->timelineSemaphoreForCopyDone.signaledValue = &record->timelineValue;
    }
    {
        const size_t index = static_cast<size_t>(m_deviceInfo->flightTracker->getCurrent().getFinalTargetImageIndex());

        if (!m_binaryInfo.handles[index].isInitialized())
        {
            m_binaryInfo.createSemaphoreDataAtIndex(*m_deviceInfo->eventBus, index);
        }

        m_inUseResources->binarySemaphoreForCopyDone =
            &m_deviceInfo->semaphoreManager->get(m_binaryInfo.handles[index])->semaphore;
    }

    m_inUseResources->queueToUse = getQueueToUse();
    {
        uint64_t signalValue = m_deviceInfo->flightTracker->getCurrent().getNumTimesFrameProcessed() + 1;
        m_inUseResources->timelineSemaphoreForCopyDone.valueToSignal = signalValue;
    }
}

vk::Queue DefaultCopyPolicy::getQueueToUse() const
{
    auto *defaultTransferQueue = core::helper::GetEngineDefaultQueue(
        *m_deviceInfo->eventBus, *m_deviceInfo->queueManager, star::Queue_Type::Ttransfer);

    if (defaultTransferQueue == nullptr)
    {
        STAR_THROW("Failed to obtain default transfer queue to use");
    }

    return defaultTransferQueue->getVulkanQueue();
}

void DefaultCopyPolicy::registerWithCommandBufferManager()
{
    m_copyCmds.init(*m_deviceInfo->device, *m_deviceInfo->commandManager);
    m_blitCmds.init(*m_deviceInfo->device, *m_deviceInfo->commandManager);
}

void DefaultCopyPolicy::SemaphoreInfo::init(const uint8_t &numFramesInFlight)
{
    handles.resize(numFramesInFlight);
    raws.resize(numFramesInFlight);
}

void DefaultCopyPolicy::SemaphoreInfo::createSemaphoreDataAtIndex(star::common::EventBus &eventBus, const size_t &index,
                                                                  std::optional<uint64_t> initialSignalValueOfTimeline)
{
    assert(index < handles.size());

    void *r = nullptr;

    core::device::manager::SemaphoreRequest request =
        initialSignalValueOfTimeline.has_value()
            ? core::device::manager::SemaphoreRequest(initialSignalValueOfTimeline.value())
            : core::device::manager::SemaphoreRequest();
    eventBus.emit(
        core::device::system::event::ManagerRequest{star::common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                                                        core::device::manager::GetSemaphoreEventTypeName),
                                                    std::move(request), handles[index], &r});

    assert(r != nullptr && handles[index].isInitialized() && "Emit did not provide a result");
    raws[index] = &static_cast<core::device::manager::SemaphoreRecord *>(r)->semaphore;
}

void DefaultCopyPolicy::initSemaphores(const uint8_t &numFramesInFlight)
{
    m_binaryInfo.init(numFramesInFlight);
    m_timelineInfo.init(numFramesInFlight);
}

StarTextures::Texture DefaultCopyPolicy::createBlitTargetTexture(const vk::Extent2D &extent) const
{
    return StarTextures::Texture::Builder(m_deviceInfo->device->getVulkanDevice(),
                                          m_deviceInfo->device->getAllocator().get())
        .build();
}

} // namespace star::service::detail::screen_capture