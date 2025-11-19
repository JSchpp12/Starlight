#pragma once

#include "CalleeRenderDependencies.hpp"
#include "DeviceInfo.hpp"
#include "wrappers/graphics/StarCommandBuffer.hpp"

namespace star::service::detail::screen_capture
{
class DefaultCopyPolicy
{
  public:
    void init(DeviceInfo &deviceInfo);

    void triggerSubmission(CalleeRenderDependencies &targetDeps, const uint8_t &frameInFlightIndex);

    void registerWithCommandBufferManager();

  private:
    Handle m_commandBuffer;
    std::vector<Handle> m_doneSemaphores;
    Handle m_startOfFrameListener;
    DeviceInfo *m_deviceInfo = nullptr;
    CalleeRenderDependencies *m_targetDeps = nullptr;

    void recordCommandBuffer(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                             const uint64_t &frameIndex);

    void recordCopyCommands(vk::CommandBuffer &commandBuffer) const;

    void addMemoryDependencies(vk::CommandBuffer &commandBuffer) const;

    std::vector<vk::ImageMemoryBarrier2> getImageBarriersForThisFrame() const;

    vk::Semaphore submitBuffer(StarCommandBuffer &buffer, const int &frameIndexToBeDrawn,
                               std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                               std::vector<vk::Semaphore> dataSemaphores,
                               std::vector<vk::PipelineStageFlags> dataWaitPoints);

    std::vector<Handle> createSemaphores(core::device::system::EventBus &eventBus, const uint8_t &numFramesInFlight);

    void registerListenerForNextFrameStart(CalleeRenderDependencies &deps, const uint8_t &frameInFlightIndex);

    void startOfFrameEventCallback(const Handle &calleeCommandBuffer, const uint8_t &targetFrameInFlightIndex,
                                   const star::common::IEvent &e, bool &keepAlive);
};
} // namespace star::service::detail::screen_capture