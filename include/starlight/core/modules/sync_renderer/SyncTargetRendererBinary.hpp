#pragma once

#include "starlight/core/modules/sync_renderer/SyncTargetRenderer.hpp"

namespace star::core::modules::sync_renderer
{
class SyncTargetRendererBinary : public SyncTargetRenderer
{
  public:
    SyncTargetRendererBinary(star::common::EventBus &eventBus,
                             core::device::manager::ManagerCommandBuffer &commandBufferManager,
                             Handle sourceCommandBuffer, Handle targetCommandBuffer, vk::Semaphore targetSemaphore,
                             uint64_t createdOnFrameCount, uint8_t targetFrameIndex);

    virtual ~SyncTargetRendererBinary() = default;

  protected:
    virtual void registerWaitWithManager() override;
};
} // namespace star::core::modules::sync_renderer