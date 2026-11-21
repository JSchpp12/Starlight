#pragma once

#include <star_common/Handle.hpp>

#include <vulkan/vulkan.hpp>

#include <array>
#include <cstdint>

namespace star::core::device
{
class DeviceContext;
}

namespace star::core::graphics
{
/// @brief Raw Vulkan timeline-sync point: the semaphore, its timeline value, and the stage mask that applies when it is
/// used as a wait.
struct SemaphoreSyncPoint
{
    uint64_t signalValue{0};
    vk::Semaphore semaphore{VK_NULL_HANDLE};
    vk::PipelineStageFlags2 where{vk::PipelineStageFlagBits2::eAllCommands};
};

/// @brief Up to two submissions a transfer must wait on before its own work may execute.
struct WorkWaitInfo
{
    std::array<SemaphoreSyncPoint, 2> waits{};
    uint8_t count{0};
};

/// @brief Completion timeline of the submission that will execute the commands currently being recorded.
struct RecordingSubmissionInfo
{
    SemaphoreSyncPoint gate;
};

/// @brief Resolves the recording submission info of a command buffer's pass registration.
RecordingSubmissionInfo GetRecordingSubmissionInfo(core::device::DeviceContext &device,
                                                   const Handle &recordingCommandBuffer);
} // namespace star::core::graphics
