#include "starlight/core/modules/sync_renderer/SyncTargetRendererTimeline.hpp"

#include <star_common/HandleTypeRegistry.hpp>

namespace star::core::modules::sync_renderer
{
// using ListenStart = star::policy::ListenForStartOfNextFramePolicy<SyncTargetRendererTimeline>;

// std::shared_ptr<SyncTargetRendererTimeline> SyncTargetRendererTimeline::Builder::buildShared()
//{
//     assert(m_commandBufferManager != nullptr && m_targetSemaphore != VK_NULL_HANDLE &&
//            m_commandBufferManager != nullptr && m_eventBus != nullptr && m_sourceCommandBuffer.isInitialized() &&
//            m_targetCommandBuffer.isInitialized());
//     assert(m_createdOnFrameCount.has_value());
//
//     return std::shared_ptr<SyncTargetRendererTimeline>(new SyncTargetRendererTimeline(
//         std::move(m_createdOnFrameCount.value()), std::move(m_signalValue), std::move(m_sourceCommandBuffer),
//         std::move(m_targetCommandBuffer), m_targetFrameInFlightIndex, m_targetSemaphore, *m_commandBufferManager,
//         *m_eventBus));
// }

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
} // namespace star::core::modules::sync_renderer