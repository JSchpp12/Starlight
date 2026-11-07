#pragma once

#include <cstdint>

#include <vulkan/vulkan.hpp>

namespace star
{
/// @brief Queue-family ownership parameters for an image memory barrier.
/// @details Both indices default to VK_QUEUE_FAMILY_IGNORED, which leaves the barrier as a plain layout transition.
/// When they differ, the barrier performs a queue-family ownership transfer: the release must be recorded on the
/// source queue and the acquire on the destination queue. A release barrier's destination stage/access masks are
/// ignored by Vulkan; its dst family still identifies the acquiring family.
struct QueueFamilyOwnership
{
    /// Family that currently owns the image.
    std::uint32_t src{VK_QUEUE_FAMILY_IGNORED};
    /// Family that acquires the image.
    std::uint32_t dst{VK_QUEUE_FAMILY_IGNORED};
};
} // namespace star
