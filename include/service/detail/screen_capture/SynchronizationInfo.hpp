#pragma once

#include "StarBuffers/Buffer.hpp"

#include <vulkan/vulkan.hpp>

namespace star::service::detail::screen_capture
{
struct SynchronizationInfo
{
    const vk::Semaphore &semaphore;
    const uint64_t &signalValue;
};
} // namespace star::service::detail::screen_capture