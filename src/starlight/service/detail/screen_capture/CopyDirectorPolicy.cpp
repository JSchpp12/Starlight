#include "service/detail/screen_capture/CopyDirectorPolicy.hpp"

#include "core/device/managers/Semaphore.hpp"
#include "core/device/system/event/ManagerRequest.hpp"
#include "event/GetQueue.hpp"
#include "logging/LoggingFactory.hpp"
#include "core/helper/queue/QueueHelpers.hpp"

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

    createSemaphores(*m_deviceInfo->eventBus,
                     m_deviceInfo->flightTracker->getSetup().getNumUniqueTargetFramesForFinalization());
}

GPUSynchronizationInfo DefaultCopyPolicy::triggerSubmission(CopyPlan &copyPlan)
{
    prepareInProgressResources(copyPlan);

    assert(m_deviceInfo->commandManager != nullptr);
    m_copyCmds.trigger(*m_deviceInfo->commandManager);

    const size_t index = static_cast<size_t>(m_deviceInfo->flightTracker->getCurrent().getFrameInFlightIndex());
    return GPUSynchronizationInfo{.signalValue = m_deviceInfo->flightTracker->getCurrent().getNumTimesFrameProcessed(),
                                  .copyCommandBuffer = m_copyCmds.getCommandBuffer(),
                                  .binarySemaphoreForMainCopyDone = *m_binaryInfo.raws[index],
                                  .timelineSemaphoreForMainCopyCommandsDone = *m_timelineInfo.raws[index]};
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
        const size_t resourceIndex =
            static_cast<size_t>(m_deviceInfo->flightTracker->getCurrent().getFinalTargetImageIndex());

        assert(resourceIndex < m_timelineInfo.raws.size() &&
               "Resource index outside of created semaphore range for copyDirector");
        auto *record = m_deviceInfo->semaphoreManager->get(m_timelineInfo.handles[resourceIndex]);
        m_inUseResources->timelineSemaphoreForCopyDone.semaphore = &record->semaphore;
        m_inUseResources->timelineSemaphoreForCopyDone.signaledValue = &record->timlineValue;

        assert(resourceIndex < m_binaryInfo.handles.size() && "Resource index outside of binary semaphore range");
        m_inUseResources->binarySemaphoreForCopyDone =
            &m_deviceInfo->semaphoreManager->get(m_binaryInfo.handles[resourceIndex])->semaphore;
    }

    m_inUseResources->queueToUse = getQueueToUse();
    {
        uint64_t signalValue = m_deviceInfo->flightTracker->getCurrent().getNumTimesFrameProcessed() + 1;
        m_inUseResources->timelineSemaphoreForCopyDone.valueToSignal = signalValue;
    }
}

vk::Queue DefaultCopyPolicy::getQueueToUse() const
{
    Handle defaultTransferQueue;

    core::helper::GetEngineDefaultQueue(*m_deviceInfo->eventBus, *m_deviceInfo->queueManager,
                                        star::Queue_Type::Ttransfer);

    if (!defaultTransferQueue.isInitialized())
    {
        STAR_THROW("Failed to obtain default transfer queue to use");
    }

    return m_deviceInfo->queueManager->get(defaultTransferQueue)->queue.getVulkanQueue();
}

void DefaultCopyPolicy::registerWithCommandBufferManager()
{
    m_copyCmds.init(*m_deviceInfo->device, *m_deviceInfo->commandManager);
    m_blitCmds.init(*m_deviceInfo->device, *m_deviceInfo->commandManager);
}

void DefaultCopyPolicy::SemaphoreInfo::init(star::common::EventBus &eventBus, const uint8_t &numFramesInFlight,
                                            bool isTimeline)
{
    handles.resize(numFramesInFlight);
    raws.resize(numFramesInFlight);

    for (uint8_t i = 0; i < numFramesInFlight; i++)
    {
        {
            void *r = nullptr;
            eventBus.emit(core::device::system::event::ManagerRequest{
                star::common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                    core::device::manager::GetSemaphoreEventTypeName),
                core::device::manager::SemaphoreRequest{isTimeline}, handles[i], &r});

            assert(r != nullptr && handles[i].isInitialized() && "Emit did not provide a result");
            raws[i] = &static_cast<core::device::manager::SemaphoreRecord *>(r)->semaphore;
        }
    }
}

void DefaultCopyPolicy::createSemaphores(star::common::EventBus &eventBus, const uint8_t &numFramesInFlight)
{
    m_binaryInfo.init(eventBus, numFramesInFlight, false);
    m_timelineInfo.init(eventBus, numFramesInFlight, true);
}

StarTextures::Texture DefaultCopyPolicy::createBlitTargetTexture(const vk::Extent2D &extent) const
{
    return StarTextures::Texture::Builder(m_deviceInfo->device->getVulkanDevice(),
                                          m_deviceInfo->device->getAllocator().get())
        .build();
}

} // namespace star::service::detail::screen_capture