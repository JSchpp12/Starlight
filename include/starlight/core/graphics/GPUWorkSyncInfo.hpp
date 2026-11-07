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
struct SemaphoreInfo
{
    uint64_t signalValue{0};
    vk::Semaphore semaphore{VK_NULL_HANDLE};
    vk::PipelineStageFlags2 where{vk::PipelineStageFlagBits2::eAllCommands};
};

struct GPUWorkSyncInfo
{
    // A resource update needs at most two dependencies: the prior transfer write and the graphics/compute submission
    // still consuming the resource.
    std::array<SemaphoreInfo, 2> workWaitOn{};
    uint8_t workWaitOnCount{0};
    SemaphoreInfo workSignalWhenDone;
};

/// @brief Completion timeline of the submission that will execute the commands currently being recorded.
/// @details Resolved once per render phase per frame from the phase's command-buffer pass registration. The gate lets
/// object setup releases wait for the recording submission.
struct RecordingSubmissionInfo
{
    SemaphoreInfo gate;
};

/// @brief Resolves the recording submission info of a command buffer's pass registration.
RecordingSubmissionInfo GetRecordingSubmissionInfo(core::device::DeviceContext &device,
                                                   const Handle &recordingCommandBuffer);
} // namespace star::core::graphics
