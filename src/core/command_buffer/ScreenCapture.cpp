#include "core/command_buffer/ScreenCapture.hpp"

namespace star::core::command_buffer
{

void ScreenCapture::prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                               const vk::Extent2D &renderingResolution)
{
    CommandBufferBase::prepRender(context, numFramesInFlight);

    m_transferDstTextures = createTransferDstTextures(context, numFramesInFlight, renderingResolution);
    m_hostVisibleBuffers = createHostVisibleBuffers(context, numFramesInFlight, renderingResolution,
                                                    m_transferDstTextures.front().getSize());
}

void ScreenCapture::cleanupRender(core::device::DeviceContext &context)
{
    cleanupIntermediateImages(context);
    cleanupBuffers(context);

    CommandBufferBase::cleanupRender(context);
}

star::Handle ScreenCapture::registerCommandBuffer(core::device::DeviceContext &context,
                                                  const uint8_t &numFramesInFlight)
{
    return Handle();
}

std::vector<StarTextures::Texture> ScreenCapture::createTransferDstTextures(
    core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
    const vk::Extent2D &renderingResolution) const
{
    auto textures = std::vector<StarTextures::Texture>();
    const vk::Format format = m_targetTextures.front().getBaseFormat();

    const auto queueFamilyInds = std::vector<uint32_t>{
        context.getDevice().getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex(),
        context.getDevice().getDefaultQueue(star::Queue_Type::Ttransfer).getParentQueueFamilyIndex()};

    for (uint8_t i = 0; i < numFramesInFlight; i++)
    {
        textures.emplace_back(
            StarTextures::Texture::Builder(context.getDevice().getVulkanDevice(),
                                           context.getDevice().getAllocator().get())
                .setBaseFormat(format)
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

std::vector<StarBuffers::Buffer> star::core::command_buffer::ScreenCapture::createHostVisibleBuffers(
    core::device::DeviceContext &context, const uint8_t &numFramesInFlight, const vk::Extent2D &renderingResolution,
    const vk::DeviceSize &bufferSize) const
{
    auto buffers = std::vector<StarBuffers::Buffer>();

    auto builder = StarBuffers::Buffer::Builder(context.getDevice().getAllocator().get())
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

void ScreenCapture::cleanupBuffers(core::device::DeviceContext &context)
{
    for (auto &buffer : m_hostVisibleBuffers)
    {
        buffer.cleanupRender(context.getDevice().getVulkanDevice());
    }
}

void ScreenCapture::cleanupIntermediateImages(core::device::DeviceContext &context)
{
    for (auto &image : m_transferDstTextures)
    {
        image.cleanupRender(context.getDevice().getVulkanDevice());
    }
}
} // namespace star::core::command_buffer