#pragma once

#include <star_common/Handle.hpp>

#include <vulkan/vulkan.hpp>

#include <array>
#include <cstdint>

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
} // namespace star::core::graphics
