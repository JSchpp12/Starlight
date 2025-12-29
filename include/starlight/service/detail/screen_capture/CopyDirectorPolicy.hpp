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
    Handle m_commandBufferTransfer, m_commandBufferGraphics;
    std::vector<Handle> m_doneSemaphoreHandles;
    std::vector<vk::Semaphore *> m_doneSemaphoresRaw;
    std::vector<Handle> m_binarySignalSemaphoresHandles;
    std::vector<vk::Semaphore *> m_binarySignalSemaphoresRaw;
    Handle m_startOfFrameListener;
    DeviceInfo *m_deviceInfo = nullptr;
    std::unique_ptr<common::InUseResourceInformation> m_inUseResources =
        std::make_unique<common::InUseResourceInformation>();

    ExecuteCmdBuffer<CopyCmdPolicy> m_copyCmds;
    ExecuteCmdBuffer<BlitCmdPolicy> m_blitCmds;

    void prepareInProgressResources(CopyPlan &extentResources) noexcept;

    void createSemaphores(star::common::EventBus &eventBus, const uint8_t &numFramesInFlight);

    void registerListenerForNextFrameStart(CalleeRenderDependencies &deps, const uint8_t &frameInFlightIndex);

    void startOfFrameEventCallback(const Handle &calleeCommandBuffer, const uint8_t &targetFrameInFlightIndex,
                                   const uint64_t &signaledSemaphoreValue, const star::common::IEvent &e,
                                   bool &keepAlive);

    StarTextures::Texture createBlitTargetTexture(const vk::Extent2D &extent) const;
};
} // namespace star::service::detail::screen_capture