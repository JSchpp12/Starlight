#include "starlight/core/waiter/sync_renderer/SyncTargetRendererBinary.hpp"

namespace star::core::waiter::sync_renderer
{
SyncTargetRendererBinary::SyncTargetRendererBinary(star::common::EventBus &eventBus,
                                                   core::device::manager::ManagerCommandBuffer &commandBufferManager,
                                                   Handle sourceCommandBuffer, Handle targetCommandBuffer,
                                                   vk::Semaphore targetSemaphore, uint64_t createdOnFrameCount,
                                                   uint8_t targetFrameIndex)
    : SyncTargetRenderer(eventBus, commandBufferManager, std::move(sourceCommandBuffer), std::move(targetCommandBuffer),
                         std::move(targetSemaphore), std::move(createdOnFrameCount), targetFrameIndex)
{
}

void SyncTargetRendererBinary::registerWaitWithManager()
{
    m_commandBufferManager.get(m_targetCommandBuffer)
        .oneTimeWaitSemaphoreInfo.insert(m_sourceCommandBuffer, m_targetSemaphore,
                                         vk::PipelineStageFlagBits::eFragmentShader);
}
} // namespace star::core::waiter::sync_renderer