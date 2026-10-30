#include "service/detail/screen_capture/PerExtentResources.hpp"

#include "StarTextures/Texture.hpp"
#include "starlight/factory/TextureFactory.hpp"
#include "starlight/maker/buffer/MakeBuffer.hpp"

#include <boost/functional/hash.hpp>

#include <cassert>

namespace star::service::detail::screen_capture
{

static star::maker::buffer::MakeBuffer CreateBufferMaker(DeviceInfo *deviceInfo, const vk::Format &targetImageFormat,
                                                         const vk::Extent2D &extent)
{
    const vk::DeviceSize size = StarTextures::TextureFactory::CalculateSize(
        targetImageFormat, vk::Extent3D().setHeight(extent.height).setWidth(extent.width).setDepth(1), 1,
        vk::ImageType::e2D, 1);

    return star::maker::buffer::MakeBuffer(*deviceInfo->device)
        .setCreateInfo(Allocator::AllocationBuilder()
                           .setFlags(VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT)
                           .setUsage(VMA_MEMORY_USAGE_AUTO_PREFER_HOST)
                           .build(),
                       vk::BufferCreateInfo()
                           .setSharingMode(vk::SharingMode::eExclusive)
                           .setSize(size)
                           .setUsage(vk::BufferUsageFlagBits::eTransferDst),
                       "PerExtentResource")
        .setInstanceCount(1)
        .setInstanceSize(size);
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

CopyResource PerExtentResources::giveMeResource(const vk::Extent2D &targetExtent)
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
                .insert(std::make_pair(targetExtent, std::make_unique<CopyResourcesContainer>(CreateBufferMaker(
                                                         m_deviceInfo, vk::Format::eR8G8B8A8Unorm, targetExtent))))
                .first->second.get();
    }

    Handle acquiredResource = container->getBufferPool().acquireBlocking();

    return CopyResource{.bufferInfo = CopyResource::ThreadSharedBufferInfo{
                            .containerRegistration = acquiredResource,
                            .hostVisibleBuffer = container->getBufferPool().get(acquiredResource),
                            .container = container}};
}

void PerExtentResources::cleanupRender()
{
}

} // namespace star::service::detail::screen_capture
