#pragma once

#include "starlight/core/modules/sync_renderer/SyncTargetRenderer.hpp"

#include <vulkan/vulkan.hpp>

namespace star::core::modules::sync_renderer
{
class SyncTargetRendererTimeline : public SyncTargetRenderer
{
  public:
    SyncTargetRendererTimeline(uint64_t signalValue, star::common::EventBus &eventBus,
                               core::device::manager::ManagerCommandBuffer &commandBufferManager,
                               Handle sourceCommandBuffer, Handle targetCommandBuffer, vk::Semaphore targetSemaphore,
                               uint64_t createdOnFrameCount, uint8_t targetFrameIndex);

    virtual ~SyncTargetRendererTimeline() = default;

  protected:
    virtual void registerWaitWithManager() override;

  private:
    friend class star::policy::ListenForStartOfNextFramePolicy<SyncTargetRendererTimeline>;

    uint64_t m_signalValue;
};
} // namespace star::core::modules::sync_renderer