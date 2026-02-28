#include "starlight/core/waiter/sync_renderer/SyncTargetRendererTimeline.hpp"

#include <star_common/HandleTypeRegistry.hpp>

namespace star::core::waiter::sync_renderer
{
SyncTargetRendererTimeline::SyncTargetRendererTimeline(
    uint64_t signalValue, star::common::EventBus &eventBus,
    core::device::manager::ManagerCommandBuffer &commandBufferManager, Handle sourceCommandBuffer,
    Handle targetCommandBuffer, vk::Semaphore targetSemaphore, uint64_t createdOnFrameCount, uint8_t targetFrameIndex)
    : SyncTargetRenderer(eventBus, commandBufferManager, std::move(sourceCommandBuffer), std::move(targetCommandBuffer),
                         std::move(targetSemaphore), std::move(createdOnFrameCount), targetFrameIndex),
      m_signalValue(std::move(signalValue))
{
}

void SyncTargetRendererTimeline::registerWaitWithManager()
{
    m_commandBufferManager.get(m_targetCommandBuffer)
        .oneTimeWaitSemaphoreInfo.insert(m_sourceCommandBuffer, m_targetSemaphore,
                                         vk::PipelineStageFlagBits::eFragmentShader, m_signalValue);
}
} // namespace star::core::waiter::sync_renderer