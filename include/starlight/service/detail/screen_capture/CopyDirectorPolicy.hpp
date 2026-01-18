#pragma once

#include "BlitCmdPolicy.hpp"
#include "CalleeRenderDependencies.hpp"
#include "Common.hpp"
#include "CopyCmdPolicy.hpp"
#include "CopyPlan.hpp"
#include "DeviceInfo.hpp"
#include "ExecuteCmdBuffer.hpp"
#include "GPUSynchronizationInfo.hpp"
#include "wrappers/graphics/StarCommandBuffer.hpp"

#include <absl/container/flat_hash_map.h>

namespace star::service::detail::screen_capture
{

class DefaultCopyPolicy
{
  public:
    DefaultCopyPolicy()
        : m_copyCmds(CopyCmdPolicy(m_inUseResources.get())), m_blitCmds(BlitCmdPolicy(m_inUseResources.get()))
    {
    }

    void init(DeviceInfo &deviceInfo);

    GPUSynchronizationInfo triggerSubmission(CopyPlan &copyPlan);

    void registerWithCommandBufferManager();

  private:
    struct SemaphoreInfo
    {
        std::vector<Handle> handles;
        std::vector<vk::Semaphore *> raws;

        void init(const uint8_t &numFramesInFlight);

        void createSemaphoreDataAtIndex(star::common::EventBus &eventBus, const size_t &index,
                                        std::optional<uint64_t> initialSignalValueIfTimeline = std::nullopt);
    };
    Handle m_commandBufferTransfer, m_commandBufferGraphics;
    SemaphoreInfo m_timelineInfo, m_binaryInfo;
    Handle m_startOfFrameListener;
    DeviceInfo *m_deviceInfo = nullptr;
    std::unique_ptr<common::InUseResourceInformation> m_inUseResources =
        std::make_unique<common::InUseResourceInformation>();

    ExecuteCmdBuffer<CopyCmdPolicy> m_copyCmds;
    ExecuteCmdBuffer<BlitCmdPolicy> m_blitCmds;

    void prepareInProgressResources(CopyPlan &extentResources) noexcept;

    void initSemaphores(const uint8_t &numFramesInFlight);

    vk::Queue getQueueToUse() const;

    StarTextures::Texture createBlitTargetTexture(const vk::Extent2D &extent) const;
};
} // namespace star::service::detail::screen_capture