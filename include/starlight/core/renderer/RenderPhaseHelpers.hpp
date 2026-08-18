#pragma once

#include <star_common/EventBus.hpp>
#include <star_common/FrameTracker.hpp>
#include <star_common/Handle.hpp>

#include <starlight/core/CommandBus.hpp>
#include <starlight/core/device/managers/ManagerCommandBuffer.hpp>
#include <starlight/core/device/managers/Queue.hpp>
#include <starlight/core/graphics/GPUWorkSyncInfo.hpp>
#include <starlight/object/StarObject.hpp>
#include <starlight/systems/StarRenderGroup.hpp>

#include <vulkan/vulkan.hpp>

#include <memory>
#include <optional>
#include <vector>

namespace star::core::device
{
class DeviceContext;
}

namespace star::core::renderer
{
/// @brief Create one timeline semaphore per frame-in-flight via the semaphore manager
/// @param evtBus
/// @param ft
/// @return
std::vector<star::Handle> CreateSemaphores(star::common::EventBus &evtBus,
                                           const star::common::FrameTracker &ft) noexcept;

/// @brief Register a command buffer with the command-order DAG on the default graphics queue.
/// @param cmdBus
/// @param evtBus
/// @param qm
/// @param commandBuffer
void RegisterWithCommandOrder(const star::core::CommandBus &cmdBus, star::common::EventBus &evtBus,
                              star::core::device::manager::Queue &qm, star::Handle commandBuffer);

/// @brief Group objects into compatible render groups
/// @param context
/// @param objects
/// @return
std::vector<star::StarRenderGroup> CreateRenderingGroups(star::core::device::DeviceContext &context,
                                                         std::vector<std::shared_ptr<star::StarObject>> objects);

/// @brief Wait on this pass's timeline semaphore if the command-order service has not yet processed the current frame
/// for it.
/// @param cmdBus
/// @param device
/// @param commandBuffer
/// @param ft
void waitForTimelineSemaphore(const star::core::CommandBus &cmdBus, vk::Device device, star::Handle commandBuffer,
                              const star::common::FrameTracker &ft);

/// @brief Build the edge-aware submission override (wraps submitEdgeAwarePass).
/// @param cmdBus
/// @param commandBuffer
/// @param signalBinaryCompletion
/// @return
std::optional<star::core::device::manager::ManagerCommandBuffer::BufferSubmissionOverride>
makeEdgeAwareSubmissionOverride(const star::core::CommandBus *cmdBus, const star::Handle *commandBuffer,
                                bool signalBinaryCompletion);

/// @brief Find the neighbor consumer's sync info via the command-order edge walk (find the edge where this pass is the
/// producer and read the consumer's signaled semaphore + current signal value).
/// @param cmdBus
/// @param myCommandBuffer
/// @return
std::optional<star::core::graphics::SemaphoreInfo> GetNeighborConsumerSyncInfo(const star::core::CommandBus &cmdBus,
                                                                               const star::Handle &myCommandBuffer);
} // namespace star::core::renderer
