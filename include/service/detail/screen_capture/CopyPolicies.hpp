#pragma once

#include "DeviceInfo.hpp"

#include "CalleeRenderDependencies.hpp"
#include "core/device/managers/ManagerCommandBuffer.hpp"
#include "core/device/system/EventBus.hpp"
#include "service/InitParameters.hpp"
#include "wrappers/graphics/StarCommandBuffer.hpp"

#include <vulkan/vulkan.hpp>

namespace star::service::detail::screen_capture
{
class DefaultCopyPolicy
{
  public:
    void init(DeviceInfo &deviceInfo, const uint8_t &numFramesInFlight);

    void triggerSubmission(CalleeRenderDependencies &targetDeps);

    void registerCalleeDependency(CalleeRenderDependencies newDependencies)
    {
        dependencies = std::move(newDependencies);
    }

    void registerWithCommandBufferManager();

  private:
    CalleeRenderDependencies dependencies = CalleeRenderDependencies();
    Handle m_commandBuffer;
    std::vector<Handle> m_doneSemaphores;
    Handle m_startOfFrameListener;
    DeviceInfo *m_deviceInfo = nullptr;

    void recordCommandBuffer(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                             const uint64_t &frameIndex);

    void addMemoryDependencies(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                               const uint64_t &frameIndex);

    vk::Semaphore submitBuffer(StarCommandBuffer &buffer, const int &frameIndexToBeDrawn,
                               std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                               std::vector<vk::Semaphore> dataSemaphores,
                               std::vector<vk::PipelineStageFlags> dataWaitPoints);

    std::vector<Handle> createSemaphores(core::device::system::EventBus &eventBus, const uint8_t &numFramesInFlight);

    void registerListenerForNextFrameStart(CalleeRenderDependencies &deps);

    void startOfFrameEventCallback(const Handle &calleeCommandBuffer,
                                   const star::common::IEvent &e, bool &keepAlive);
};
} // namespace star::service::detail::screen_capture