#pragma once

#include <vulkan/vulkan.hpp>

namespace star
{
struct StarSemaphore
{
    vk::Semaphore vkSemaphore{VK_NULL_HANDLE};
    uint64_t signalValue{0};
};
} // namespace star