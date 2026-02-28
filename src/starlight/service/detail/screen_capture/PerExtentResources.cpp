#include "service/detail/screen_capture/PerExtentResources.hpp"

#include "wrappers/graphics/policies/GenericBufferCreateAllocatePolicy.hpp"
#include "wrappers/graphics/policies/GenericImageCreateAllocatePolicy.hpp"

#include <boost/functional/hash.hpp>

#include <cassert>

namespace star::service::detail::screen_capture
{

static wrappers::graphics::policies::GenericImageCreateAllocatePolicy CreateImagePolicy(
    DeviceInfo *deviceInfo, const vk::Format &targetImageFormat, const vk::Extent2D &extent)
{
    const vk::Extent3D imageExtent = vk::Extent3D().setHeight(extent.height).setWidth(extent.width).setDepth(1);

    return wrappers::graphics::policies::GenericImageCreateAllocatePolicy{
        .extent = imageExtent,
        .format = targetImageFormat,
        .allocationName = "PerImageExtentResource_TargetBlit",
        .allocationCreateInfo = Allocator::AllocationBuilder()
                                    .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                                    .setUsage(VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO)
                                    .build(),
        .allocator = deviceInfo->device->getAllocator().get(),
        .device = deviceInfo->device->getVulkanDevice()};
}

static std::vector<star::StarTextures::Texture> CreateImages(DeviceInfo *deviceInfo,
                                                             const vk::Format &targetImageFormat,
                                                             const vk::Extent2D &extent)
{
    assert(deviceInfo != nullptr && deviceInfo->flightTracker != nullptr);
    assert(deviceInfo->queueManager != nullptr);

    auto policy = CreateImagePolicy(deviceInfo, targetImageFormat, extent);
    const size_t numTargetImages =
        static_cast<size_t>(deviceInfo->flightTracker->getSetup().getNumUniqueTargetFramesForFinalization());
    std::vector<StarTextures::Texture> result = std::vector<StarTextures::Texture>(numTargetImages);
    for (size_t i{0}; i < numTargetImages; i++)
    {
        result[i] = policy.create();
    }

    return result;
}

static wrappers::graphics::policies::GenericBufferCreateAllocatePolicy CreateBufferPolicy(
    DeviceInfo *deviceInfo, const vk::Format &targetImageFormat, const vk::Extent2D &extent)
{
    const vk::DeviceSize size = StarTextures::Texture::CalculateSize(
        targetImageFormat, vk::Extent3D().setHeight(extent.height).setWidth(extent.width).setDepth(1), 1,
        vk::ImageType::e2D, 1);
    return wrappers::graphics::policies::GenericBufferCreateAllocatePolicy{
        .allocator = std::move(deviceInfo->device->getAllocator().get()),
        .allocationName = "PerExtentResource",
        .allocInfo = Allocator::AllocationBuilder()
                         .setFlags(VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT)
                         .setUsage(VMA_MEMORY_USAGE_AUTO_PREFER_HOST)
                         .build(),
        .createInfo = vk::BufferCreateInfo()
                          .setSharingMode(vk::SharingMode::eExclusive)
                          .setSize(size)
                          .setUsage(vk::BufferUsageFlagBits::eTransferDst),
        .instanceSize = size,
        .instanceCount = 1};
}

std::size_t Extent2DHash::operator()(const vk::Extent2D &e) const noexcept
{
    std::size_t seed = 0;
    boost::hash_combine(seed, e.width);
    boost::hash_combine(seed, e.height);
    return seed;
}

bool Extent2DEqual::operator()(const vk::Extent2D &a, const vk::Extent2D &b) const noexcept
{
    return a.width == b.width && a.height == b.height;
}

CopyResource PerExtentResources::giveMeResource(const vk::Extent2D &targetExtent, const Handle &calleeRegistration,
                                                const uint8_t &frameInFlightIndex)
{
    assert(m_deviceInfo != nullptr);
    CopyResourcesContainer *container = nullptr;

    if (m_resources.contains(targetExtent))
    {
        container = m_resources.at(targetExtent).get();
    }
    else
    {
        container =
            m_resources
                .insert(std::make_pair(targetExtent, std::make_unique<CopyResourcesContainer>(CreateBufferPolicy(
                                                         m_deviceInfo, vk::Format::eR8G8B8A8Unorm, targetExtent))))
                .first->second.get();
    }

    auto &calleeTextures = container->getBlitTexturePool().get(calleeRegistration);
    if (calleeTextures.textures.size() == 0)
    {
        calleeTextures.textures = CreateImages(m_deviceInfo, vk::Format::eR8G8B8A8Unorm, targetExtent);
    }

    Handle acquiredResource = container->getBufferPool().acquireBlocking();

    return CopyResource{
        .bufferInfo =
            CopyResource::ThreadSharedBufferInfo{.containerRegistration = acquiredResource,
                                                 .hostVisibleBuffer = container->getBufferPool().get(acquiredResource),
                                                 .container = container},
        .blitTargetTexture = calleeTextures.textures[frameInFlightIndex].getVulkanImage()};
}

void PerExtentResources::cleanupRender()
{
    assert(m_deviceInfo != nullptr);
    for (auto &container : m_resources)
    {
        container.second->cleanupRender(*m_deviceInfo->device);
    }
}

} // namespace star::service::detail::screen_capture