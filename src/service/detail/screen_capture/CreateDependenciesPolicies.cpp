#include "service/detail/screen_capture/CreateDependenciesPolicies.hpp"

#include <cassert>

namespace star::service::detail::screen_capture
{
std::vector<StarTextures::Texture> DefaultCreatePolicy::createTransferDstTextures(
    core::device::StarDevice &device, const uint8_t &numFramesInFlight, const vk::Extent2D &renderingResolution,
    const vk::Format &targetImageBaseFormat)
{
    auto textures = std::vector<StarTextures::Texture>();

    const auto queueFamilyInds =
        std::vector<uint32_t>{device.getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex(),
                              device.getDefaultQueue(star::Queue_Type::Ttransfer).getParentQueueFamilyIndex()};

    for (uint8_t i = 0; i < numFramesInFlight; i++)
    {
        textures.emplace_back(
            StarTextures::Texture::Builder(device.getVulkanDevice(), device.getAllocator().get())
                .setBaseFormat(targetImageBaseFormat)
                .setCreateInfo(
                    Allocator::AllocationBuilder()
                        .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                        .setUsage(VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO)
                        .build(),
                    vk::ImageCreateInfo()
                        .setExtent(vk::Extent3D()
                                       .setWidth(renderingResolution.width)
                                       .setHeight(renderingResolution.height)
                                       .setDepth(1))
                        .setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc)
                        .setImageType(vk::ImageType::e2D)
                        .setMipLevels(1)
                        .setArrayLayers(1)
                        .setTiling(vk::ImageTiling::eOptimal)
                        .setInitialLayout(vk::ImageLayout::eUndefined)
                        .setSamples(vk::SampleCountFlagBits::e1)
                        .setSharingMode(vk::SharingMode::eConcurrent)
                        .setQueueFamilyIndexCount(2)
                        .setPQueueFamilyIndices(queueFamilyInds.data()),
                    "ScreenCapture_IntermediateTransferTexture")
                .build());
    }

    return textures;
}

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
                                                     const Handle &targetTextureReadySemaphore)
{
    assert(deviceInfo.device != nullptr);

    return CalleeRenderDependencies{.commandBufferContainingTarget = commandBufferContainingTarget,
                                    .targetTextureReadySemaphore = targetTextureReadySemaphore,
                                    .targetTexture = targetTexture,
                                    .hostVisibleBuffers = createHostVisibleBuffers(*deviceInfo.device, 1,
                                                                                   deviceInfo.surface->getResolution(),
                                                                                   targetTexture.getSize())};
}

} // namespace star::service::detail::screen_capture