#pragma once

#include "StarBuffers/Buffer.hpp"

#include <vulkan/vulkan.hpp>
#include <boost/atomic/atomic.hpp>

namespace star::service::detail::screen_capture
{
struct GPUSynchronizationInfo
{
    const vk::Semaphore &semaphore;
    const uint64_t &signalValue;
};
} // namespace star::service::detail::screen_capture