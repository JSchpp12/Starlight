#pragma once

#include <star_common/FrameTracker.hpp>
#include <star_common/Handle.hpp>

#include <vulkan/vulkan.hpp>

#include <optional>
#include <vector>

namespace star
{
class StarCommandBuffer;
class StarQueue;
namespace core
{
class CommandBus;
}
} // namespace star

namespace star::core::renderer
{
/// Submit a command buffer for an edge-aware render phase against the
/// command-order DAG. Waits on the timeline semaphores of every neighbor pass:
///   - edges where this pass is the producer  -> wait on the consumer's
///     currentSignalValue (back-pressure: don't overwrite output the consumer
///     is still reading);
///   - edges where this pass is the consumer  -> wait on the producer's
///     toSignalValue (data-readiness: don't read before it is written);
/// plus any data/transfer semaphores handed in by the command-buffer manager.
/// Always signals this pass's own timeline semaphore. When
/// signalBinaryCompletion is true, also signals (and returns) the command
/// buffer's binary completion semaphore; otherwise returns an empty semaphore.
vk::Semaphore submitEdgeAwarePass(const star::core::CommandBus &cmdBus, star::Handle commandBuffer,
                                  star::StarCommandBuffer &buffer, const star::common::FrameTracker &frameTracker,
                                  std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                                  std::vector<vk::Semaphore> &dataSemaphores,
                                  std::vector<vk::PipelineStageFlags> &dataWaitPoints,
                                  std::vector<std::optional<uint64_t>> &previousSignaledValues, star::StarQueue &queue,
                                  bool signalBinaryCompletion);
} // namespace star::core::renderer
