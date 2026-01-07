#pragma once

#include "starlight/core/device/managers/ManagerCommandBuffer.hpp"
#include "starlight/core/modules/sync_renderer/SyncTargetRenderer.hpp"

#include <star_common/Handle.hpp>
#include <star_common/EventBus.hpp>

#include <optional>
#include <vulkan/vulkan.hpp>

namespace star::core::modules::sync_renderer
{
class Factory
{
  public:
    Factory(star::common::EventBus &eventBus, core::device::manager::ManagerCommandBuffer &commandBufferManager);
    Factory &setWaitPipelineStage(vk::PipelineStageFlags flags); 
    Factory &setCreatedOnFrameCount(uint64_t currentFrameCounter);
    Factory &setSemaphore(vk::Semaphore semaphore);
    Factory &setSemaphoreSignalValue(uint64_t signalValue);
    Factory &setTargetFrameInFlightIndex(uint8_t index);
    Factory &setTargetCommandBuffer(Handle commandBuffer);
    Factory &setSourceCommandBuffer(Handle commandBuffer);
    std::shared_ptr<SyncTargetRenderer> build();

  private:
    star::common::EventBus &m_eventBus; 
    core::device::manager::ManagerCommandBuffer &m_commandBufferManager;
    std::optional<uint64_t> m_signalValue = std::nullopt;
    std::optional<uint64_t> m_createdOnFrameCount = std::nullopt;
    Handle m_targetCommandBuffer;
    Handle m_sourceCommandBuffer;
    uint8_t m_targetFrameInFlightIndex;
    vk::PipelineStageFlags m_flags;
    vk::Semaphore m_targetSemaphore = VK_NULL_HANDLE;
};
} // namespace star::core::modules::sync_renderer