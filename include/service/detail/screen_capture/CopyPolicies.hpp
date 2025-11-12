#pragma once

#include "CalleeRenderDependencies.hpp"

#include <vulkan/vulkan.hpp>

namespace star::service::detail::screen_capture
{
struct DefaultCopyPolicy
{
    void recordCommandBuffer(CalleeRenderDependencies &deps, vk::CommandBuffer &commandBuffer,
                             const uint8_t &frameInFlightIndex, const uint64_t &frameIndex);

    void addMemoryDependencies(CalleeRenderDependencies &deps, vk::CommandBuffer &commandBuffer,
                               const uint8_t &frameInFlightIndex, const uint64_t &frameIndex);
};
} // namespace star::service::detail::screen_capture