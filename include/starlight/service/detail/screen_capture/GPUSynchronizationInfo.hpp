#pragma once

#include "StarBuffers/Buffer.hpp"
#include <star_common/Handle.hpp>

#include <vulkan/vulkan.hpp>
#include <boost/atomic/atomic.hpp>

namespace star::service::detail::screen_capture
{
struct GPUSynchronizationInfo
{
    uint64_t signalValue;
    Handle &copyCommandBuffer;
    const vk::Semaphore &binarySemaphoreForMainCopyDone;
    const vk::Semaphore &timelineSemaphoreForMainCopyCommandsDone;
};
} // namespace star::service::detail::screen_capture