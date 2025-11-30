#include "service/detail/screen_capture/CreateDependenciesPolicies.hpp"

#include <cassert>

namespace star::service::detail::screen_capture
{

std::vector<star::StarBuffers::Buffer> DefaultCreatePolicy::createHostVisibleBuffers(
    core::device::StarDevice &device, const uint8_t &numFramesInFlight, const vk::Extent2D &renderingResolution,
    const vk::DeviceSize &bufferSize)
{
    auto buffers = std::vector<StarBuffers::Buffer>();

    auto builder = StarBuffers::Buffer::Builder(device.getAllocator().get())
                       .setAllocationCreateInfo(Allocator::AllocationBuilder()
                                                    .setFlags(VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
                                                              VMA_ALLOCATION_CREATE_MAPPED_BIT)
                                                    .setUsage(VMA_MEMORY_USAGE_AUTO_PREFER_HOST)
                                                    .build(),
                                                vk::BufferCreateInfo()
                                                    .setSharingMode(vk::SharingMode::eExclusive)
                                                    .setSize(bufferSize)
                                                    .setUsage(vk::BufferUsageFlagBits::eTransferDst),
                                                "ScreenCapture_DstBuffer")
                       .setInstanceCount(uint32_t{1})
                       .setInstanceSize(bufferSize);

    for (uint8_t i = 0; i < numFramesInFlight; i++)
    {
        buffers.emplace_back(builder.build());
    }

    return buffers;
}

CalleeRenderDependencies DefaultCreatePolicy::create(DeviceInfo &deviceInfo, StarTextures::Texture targetTexture,
                                                     const Handle &commandBufferContainingTarget,
                                                     const Handle *targetTextureReadySemaphore)
{
    assert(deviceInfo.device != nullptr);

    auto builder = CalleeRenderDependencies::Builder();
    if (targetTextureReadySemaphore != nullptr){
        builder.setTargetTextureReadySemaphore(*targetTextureReadySemaphore);
    }
    return builder.setCommandBufferContainingTarget(commandBufferContainingTarget)
        .setTargetTexture(std::move(targetTexture))
        .build();
}

} // namespace star::service::detail::screen_capture