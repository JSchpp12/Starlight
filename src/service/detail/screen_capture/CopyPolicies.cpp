#include "service/detail/screen_capture/CopyPolicies.hpp"

namespace star::service::detail::screen_capture
{
void DefaultCopyPolicy::recordCommandBuffer(CalleeRenderDependencies &deps, vk::CommandBuffer &commandBuffer,
                                            const uint8_t &frameInFlightIndex, const uint64_t &frameIndex)
{
}

void DefaultCopyPolicy::addMemoryDependencies(CalleeRenderDependencies &deps, vk::CommandBuffer &commandBuffer,
                                              const uint8_t &frameInFlightIndex, const uint64_t &frameIndex)
{
}
} // namespace star::service::detail::screen_capture