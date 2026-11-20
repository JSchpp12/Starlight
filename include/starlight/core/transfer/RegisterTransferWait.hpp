#pragma once

#include <starlight/core/transfer/ReadinessFailurePolicy.hpp>

#include <vector>
#include <vulkan/vulkan.hpp>

namespace star
{
struct Handle;
struct ReadinessCondition;

namespace core::device
{
class DeviceContext;
}

namespace setup
{
struct TransferWait;
}
} // namespace star

namespace star::core::transfer
{
/// @brief Register a pass's one-time GPU wait on a transfer and ensure the transfer is CPU-published before the
/// finalization buffer records.
/// @param context Device context that owns the pass's command buffer and the engine command bus.
/// @param pass Command buffer of the pass that consumes the transferred resource.
/// @param resource Image-manager, Buffer-manager, or legacy `"stBuffer"` ManagerRenderResource handle.
/// @param semaphoreRecord Semaphore-manager record of the transfer timeline the pass must wait on. The synchronizer
/// resolves the vk::Semaphore and timeline value from the record when the pass submits.
/// @param stages Pipeline stages that consume the resource.
void RegisterTransferWait(core::device::DeviceContext &context, Handle pass, Handle resource, Handle semaphoreRecord,
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
