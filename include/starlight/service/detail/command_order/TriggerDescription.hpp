#pragma once

#include <star_common/Handle.hpp>

#include <vulkan/vulkan_handles.hpp>

#include <cstdint>
#include <variant>

namespace star::service::command_order
{
struct TriggerDescription
{
    struct BinarySemaphore
    {
        vk::Semaphore semaphore{VK_NULL_HANDLE};
    };
    struct TimelineSemaphore
    {
        Handle record{};
        uint64_t signalValue{0};
    };

    Handle passDefinition;
    std::variant<BinarySemaphore, TimelineSemaphore> semaphoreInfo{BinarySemaphore{}};
};
} // namespace star::service::command_order