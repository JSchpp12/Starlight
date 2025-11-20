#pragma once

#include "CalleeRenderDependencies.hpp"
#include "DeviceInfo.hpp"
#include "SynchronizationInfo.hpp"
#include "wrappers/graphics/StarCommandBuffer.hpp"

namespace star::service::detail::screen_capture
{
class DefaultCopyPolicy
{
  public:
    void init(DeviceInfo &deviceInfo);

    SynchronizationInfo triggerSubmission(CalleeRenderDependencies &targetDeps, const uint8_t &frameInFlightIndex);

    void registerWithCommandBufferManager();

  private:
    Handle m_commandBuffer;
    std::vector<Handle> m_doneSemaphoreHandles;
    std::vector<vk::Semaphore *> m_doneSemaphoresRaw;
    std::vector<Handle> m_binarySignalSemaphoresHandles;
    std::vector<vk::Semaphore *> m_binarySignalSemaphoresRaw;
    Handle m_startOfFrameListener;
    DeviceInfo *m_deviceInfo = nullptr;
    CalleeRenderDependencies *m_targetDeps = nullptr;

    void recordCommandBuffer(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                             const uint64_t &frameIndex);

    void recordCopyCommands(vk::CommandBuffer &commandBuffer) const;

    void addMemoryDependenciesToPrepForCopy(vk::CommandBuffer &commandBuffer) const;

    void addMemoryDependenciesToCleanupFromCopy(vk::CommandBuffer &commandBuffer) const;

    std::vector<vk::ImageMemoryBarrier2> getImageBarriersForPrep() const;

    std::vector<vk::ImageMemoryBarrier2> getImageBarriersForCleanup() const;

    vk::Semaphore submitBuffer(StarCommandBuffer &buffer, const int &frameIndexToBeDrawn,
                               std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                               std::vector<vk::Semaphore> dataSemaphores,
                               std::vector<vk::PipelineStageFlags> dataWaitPoints,
                               std::vector<std::optional<uint64_t>> previousSignaledValues);

    void createSemaphores(core::device::system::EventBus &eventBus, const uint8_t &numFramesInFlight);

    void registerListenerForNextFrameStart(CalleeRenderDependencies &deps, const uint8_t &frameInFlightIndex);

    void startOfFrameEventCallback(const Handle &calleeCommandBuffer, const uint8_t &targetFrameInFlightIndex,
                                   const uint64_t &signaledSemaphoreValue, const star::common::IEvent &e,
                                   bool &keepAlive);
};
} // namespace star::service::detail::screen_capture