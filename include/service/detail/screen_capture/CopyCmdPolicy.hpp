#pragma once

#include "Common.hpp"
#include "core/device/managers/ManagerCommandBuffer.hpp"

#include <starlight/common/Handle.hpp>

namespace star::service::detail::screen_capture
{
class CopyCmdPolicy
{
  public:
    explicit CopyCmdPolicy(common::InUseResourceInformation *inUseInfo) : m_inUseInfo(inUseInfo)
    {
    }

    Handle registerWithManager(core::device::StarDevice &device, core::device::manager::ManagerCommandBuffer &manCmdBuf);

  private:
    common::InUseResourceInformation *m_inUseInfo = nullptr;

    void recordCommandBuffer(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                             const uint64_t &frameIndex);
    void recordCopyCommands(vk::CommandBuffer &commandBuffer) const;
    void recordCopyImageToBuffer(vk::CommandBuffer &commandBuffer, vk::Image targetSrcImage) const;
    void addMemoryDependenciesToPrepForCopy(vk::CommandBuffer &commandBuffer);
    void addMemoryDependenciesToCleanupFromCopy(vk::CommandBuffer &commandBuffer);
    std::vector<vk::ImageMemoryBarrier2> getImageBarriersForPrep() const;
    std::vector<vk::ImageMemoryBarrier2> getImageBarriersForCleanup() const;

    vk::Semaphore submitBuffer(StarCommandBuffer &buffer, const int &frameIndexToBeDrawn,
                               std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                               std::vector<vk::Semaphore> dataSemaphores,
                               std::vector<vk::PipelineStageFlags> dataWaitPoints,
                               std::vector<std::optional<uint64_t>> previousSignaledValues);
};
} // namespace star::service::detail::screen_capture