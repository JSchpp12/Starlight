#include "service/detail/screen_capture/CreateDependenciesPolicies.hpp"

#include "starlight/core/Exceptions.hpp"
#include "starlight/maker/buffer/MakeBuffer.hpp"

#include <cassert>
#include <vector>

namespace star::service::detail::screen_capture
{

std::vector<star::StarBuffers::Buffer> DefaultCreatePolicy::createHostVisibleBuffers(
    core::device::StarDevice &device, const uint8_t &numFramesInFlight, const vk::Extent2D &renderingResolution,
    const vk::DeviceSize &bufferSize)
{
    auto buffers = std::vector<StarBuffers::Buffer>(numFramesInFlight);

    auto builder = star::maker::buffer::MakeBuffer(device)
                       .setCreateInfo(Allocator::AllocationBuilder()
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

    const auto makeResult = builder.make(buffers.data(), numFramesInFlight);
    if (!makeResult)
    {
        STAR_THROW("Failed to create screen capture dst buffers: " + makeResult.error().detail);
    }

    return buffers;
}

CalleeRenderDependencies DefaultCreatePolicy::create(DeviceInfo &deviceInfo, StarTextures::Texture targetTexture,
                                                     const Handle &commandBufferContainingTarget,
                                                     vk::Semaphore targetTextureReadySemaphore, uint64_t signalValue)
{
    assert(deviceInfo.device != nullptr);

    auto builder = CalleeRenderDependencies::Builder();
    if (targetTextureReadySemaphore != VK_NULL_HANDLE)
    {
        builder.setTargetTextureReadySemaphore(targetTextureReadySemaphore, signalValue);
    }
    return builder.setCommandBufferContainingTarget(commandBufferContainingTarget)
        .setTargetTexture(std::move(targetTexture))
        .build();
}

} // namespace star::service::detail::screen_capture