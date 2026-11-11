#pragma once

#include <star_common/Handle.hpp>

#include <cstdint>
#include <vulkan/vulkan.hpp>

namespace star::core::device
{
class DeviceContext;
}

namespace star::core::transfer
{
/// @brief Register a pass's one-time GPU wait on a transfer and make the pass block until that transfer is
/// CPU-published (its submit2 enqueued) before it records.
/// @details The GPU timeline wait is still what orders the transfer against the pass; the CPU wait only guarantees
/// that the transfer's submission -- and therefore the timeline signal the wait depends on -- exists before the pass
/// submits. This replaces the former global high-priority wait that ran before the command-buffer update.
/// @param context Device context that owns the pass's command buffer and the engine command bus.
/// @param pass Command buffer of the pass that consumes the transferred resource.
/// @param resource Image-manager, Buffer-manager, or legacy `"stBuffer"` ManagerRenderResource handle.
/// @param semaphore Transfer timeline the pass must wait on.
/// @param signalValue Timeline value the pass must wait for.
/// @param stages Pipeline stages that consume the resource.
void RegisterTransferWait(core::device::DeviceContext &context, Handle pass, Handle resource, vk::Semaphore semaphore,
                          uint64_t signalValue,
                          vk::PipelineStageFlags stages = vk::PipelineStageFlagBits::eVertexShader |
                                                          vk::PipelineStageFlagBits::eFragmentShader);
} // namespace star::core::transfer
