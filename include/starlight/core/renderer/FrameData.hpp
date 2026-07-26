#pragma once

#include "core/graphics/GPUWorkSyncInfo.hpp"

#include <star_common/Handle.hpp>

#include <vulkan/vulkan.hpp>

#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace star::ManagerController::RenderResource
{
class Buffer;
}

namespace star::core::device
{
class DeviceContext;
}

namespace star::core::renderer
{
/// Per-frame, phase-shared managed buffers (camera, lights, light-list, ...)
/// and their per-frame submit. This is the resource side only -- the descriptor
/// sets themselves live in StarShaderInfo / StarDescriptorSetLayout, built by
/// the phase from FrameData's controllers. FrameData owns no descriptor set.
class FrameData
{
  public:
    /// One controller's worth of per-frame sync that the owning phase must wire
    /// into its command buffer before the dependent draw/dispatch runs.
    struct PendingWait
    {
        Handle handle;
        uint64_t signalValue;
        vk::Semaphore semaphore;
        vk::PipelineStageFlags waitStage;
    };

    struct FrameUpdateResult
    {
        std::span<const PendingWait> waits;
    };

    /// Register a per-frame shared managed buffer
    FrameData &add(std::shared_ptr<ManagerController::RenderResource::Buffer> controller);

    /// Prep each controller (allocates their per-frame-in-flight handles) and
    /// reserves the pending-wait scratch so frameUpdate never allocates.
    void prepRender(core::device::DeviceContext &context, uint8_t numFramesInFlight);

    /// Submit each controller's per-frame update and collect the sync the owner
    /// must wire. Returns a non-owning span over this FrameData's scratch.
    /// `priorSync` (default none) lets an owner thread phase-specific sync
    /// (e.g. the offscreen compute-neighbor SemaphoreInfo) into the transfers.
    FrameUpdateResult frameUpdate(core::device::DeviceContext &context,
                                  std::optional<core::graphics::SemaphoreInfo> priorSync = std::nullopt);

    std::shared_ptr<ManagerController::RenderResource::Buffer> controllerAt(size_t i) const;

  private:
    std::vector<std::shared_ptr<ManagerController::RenderResource::Buffer>> m_controllers;
    std::vector<PendingWait> m_pending;
};
} // namespace star::core::renderer