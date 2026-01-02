#pragma once

#include "StarBuffers/Buffer.hpp"
#include <star_common/Handle.hpp>

#include <vulkan/vulkan.hpp>
#include <boost/atomic/atomic.hpp>

namespace star::service::detail::screen_capture
{
struct GPUSynchronizationInfo
{
    Handle &copyCommandBuffer;
    vk::Semaphore &signalSemaphore;
    const vk::Semaphore &semaphore;
    const uint64_t &signalValue;
};
} // namespace star::service::detail::screen_capture