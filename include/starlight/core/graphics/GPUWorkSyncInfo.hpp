#pragma once

#include <star_common/Handle.hpp>

#include <vulkan/vulkan.hpp>

#include <optional>

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
    std::optional<SemaphoreInfo> workWaitOn{std::nullopt};
    SemaphoreInfo workSignalWhenDone; 
};
} // namespace star::core::graphics