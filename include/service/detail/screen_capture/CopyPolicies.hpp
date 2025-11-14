#pragma once

#include "CalleeRenderDependencies.hpp"
#include "core/device/managers/ManagerCommandBuffer.hpp"
#include "core/device/system/EventBus.hpp"
#include "wrappers/graphics/StarCommandBuffer.hpp"

#include <vulkan/vulkan.hpp>

namespace star::service::detail::screen_capture
{
class DefaultCopyPolicy
{
  public:
    void init(core::device::system::EventBus &eventBus, const uint8_t &numFramesInFlight);

    void triggerSubmission(core::device::manager::ManagerCommandBuffer &commandManager);

    void registerCalleeDependency(CalleeRenderDependencies newDependencies)
    {
        dependencies = std::move(newDependencies);
    }

    void registerWithCommandBufferManager(core::device::StarDevice &device,
                                            core::device::manager::ManagerCommandBuffer &commandManager,
                                            const uint64_t &currentFrameIndex);

  private:
    CalleeRenderDependencies dependencies = CalleeRenderDependencies();
    Handle m_commandBuffer; 
    std::vector<Handle> m_doneSemaphores;

    void recordCommandBuffer(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                             const uint64_t &frameIndex);

    void addMemoryDependencies(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                               const uint64_t &frameIndex);

    vk::Semaphore submitBuffer(StarCommandBuffer &buffer, const int &frameIndexToBeDrawn,
                      std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                      std::vector<vk::Semaphore> dataSemaphores, std::vector<vk::PipelineStageFlags> dataWaitPoints);

    std::vector<Handle> createSemaphores(core::device::system::EventBus &eventBus, const uint8_t &numFramesInFlight);
};
} // namespace star::service::detail::screen_capture