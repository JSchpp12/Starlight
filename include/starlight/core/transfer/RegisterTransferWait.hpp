#pragma once

#include <starlight/core/transfer/ReadinessCondition.hpp>
#include <starlight/core/transfer/ReadinessFailurePolicy.hpp>
#include <starlight/setup/RenderResourceSetup.hpp>

#include <star_common/Handle.hpp>

#include <cstdint>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace star::core::device
{
class DeviceContext;
}

namespace star::core::transfer
{
/// @brief Register a pass's one-time GPU wait on a transfer and ensure the transfer is CPU-published before the
/// finalization buffer records.
/// @details The GPU timeline wait orders the transfer against the consuming pass. The CPU wait is registered in
/// ManagerCommandBuffer's before-finalization container, which is drained immediately before the finalization phase
/// records. This keeps the host stall off every intermediate pass while still guaranteeing the timeline signal exists
/// before the first binary-semaphore wait is submitted.
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

/// @brief Register explicitly typed readiness conditions on a pass, with a failure policy.
void RegisterPassConditions(core::device::DeviceContext &context, Handle pass,
                            std::vector<ReadinessCondition> conditions,
                            ReadinessFailurePolicy onFailure = ReadinessFailurePolicy::BlockUntilReady);

/// @brief Build the readiness condition for a transfer wait, including its GPU timeline wait.
/// @details Infers Image/Buffer/legacy readiness from the resource handle type.
[[nodiscard]] ReadinessCondition MakeTransferReadinessCondition(core::device::DeviceContext &context,
                                                                const setup::TransferWait &wait);
} // namespace star::core::transfer
