#pragma once

#include "core/device/managers/ManagerCommandBuffer.hpp"
#include "core/device/managers/Semaphore.hpp"
#include "service/detail/screen_capture/Common.hpp"
#include "service/detail/screen_capture/DeviceInfo.hpp"

#include <star_common/FrameTracker.hpp>
#include <star_common/Handle.hpp>

namespace star::service::detail::screen_capture
{
class CopyCmdPolicy
{
  public:
    explicit CopyCmdPolicy(common::InUseResourceInformation *inUseInfo) : m_inUseInfo(inUseInfo)
    {
    }

    Handle registerWithManager(core::device::StarDevice &device,
                               core::device::manager::ManagerCommandBuffer &manCmdBuf);

    void init(core::device::StarDevice &device); 

  private:
    common::InUseResourceInformation *m_inUseInfo = nullptr;
    core::device::StarDevice *m_device = nullptr;

    void recordCommandBuffer(StarCommandBuffer &commandBuffer, const star::common::FrameTracker &frameTracker,
                             const uint64_t &frameIndex);
    void recordCopyCommands(vk::CommandBuffer &commandBuffer) const;
    void recordCopyImageToBuffer(vk::CommandBuffer &commandBuffer, vk::Image targetSrcImage) const;
    void addMemoryDependenciesToPrepForCopy(vk::CommandBuffer &commandBuffer);
    void addMemoryDependenciesToCleanupFromCopy(vk::CommandBuffer &commandBuffer);
    std::vector<vk::ImageMemoryBarrier2> getImageBarriersForPrep() const;
    std::vector<vk::ImageMemoryBarrier2> getImageBarriersForCleanup() const;

    void waitForSemaphoreIfNecessary(const star::common::FrameTracker &frameTracker) const; 

    vk::Semaphore submitBuffer(StarCommandBuffer &buffer, const star::common::FrameTracker &frameTracker,
                               std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                               std::vector<vk::Semaphore> dataSemaphores,
                               std::vector<vk::PipelineStageFlags> dataWaitPoints,
                               std::vector<std::optional<uint64_t>> previousSignaledValues);
};
} // namespace star::service::detail::screen_capture