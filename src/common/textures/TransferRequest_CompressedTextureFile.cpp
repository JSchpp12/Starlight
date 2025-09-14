#include "TransferRequest_CompressedTextureFile.hpp"

#include "FileHelpers.hpp"

#include <assert.h>

star::TransferRequest::CompressedTextureFile::CompressedTextureFile(
    const uint32_t &graphicsQueueFamilyIndex, const vk::PhysicalDeviceProperties &deviceProperties,
    std::shared_ptr<SharedCompressedTexture> compressedTexture, const uint8_t &mipMapIndex)
    : compressedTexture(compressedTexture), mipMapIndex(mipMapIndex), deviceProperties(deviceProperties),
      graphicsQueueFamilyIndex(graphicsQueueFamilyIndex)
{
}

std::unique_ptr<star::StarBuffers::Buffer> star::TransferRequest::CompressedTextureFile::createStagingBuffer(
    vk::Device &device, VmaAllocator &allocator) const
{
    boost::unique_lock<boost::mutex> lock;
    ktxTexture2 *texture = nullptr;
    this->compressedTexture->giveMeTranscodedImage(lock, texture);

    return star::StarBuffers::Buffer::Builder(allocator)
        .setInstanceCount(1)
        .setInstanceSize(texture->dataSize)
        .setAllocationCreateInfo(
            VmaAllocationCreateInfo{.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
                                             VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                                    .usage = VMA_MEMORY_USAGE_AUTO},
            vk::BufferCreateInfo()
                .setSize(texture->dataSize)
                .setSharingMode(vk::SharingMode::eExclusive)
                .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
            "CompressedTexture_TransferSRCBuffer")
        .build();
}

std::unique_ptr<star::StarTextures::Texture> star::TransferRequest::CompressedTextureFile::createFinal(
    vk::Device &device, VmaAllocator &allocator, const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    boost::unique_lock<boost::mutex> lock;
    ktxTexture2 *texture = nullptr;
    this->compressedTexture->giveMeTranscodedImage(lock, texture);

    std::vector<uint32_t> indices = std::vector<uint32_t>{uint32_t(this->graphicsQueueFamilyIndex)};
    for (const auto &index : transferQueueFamilyIndex)
        indices.push_back(index);

    uint32_t numIndices = 0; 
    CastHelpers::SafeCast<size_t, uint32_t>(indices.size(), numIndices); 

    return StarTextures::Texture::Builder(device, allocator)
        .setCreateInfo(
            Allocator::AllocationBuilder()
                .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                .setUsage(VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::ImageCreateInfo()
                .setExtent(vk::Extent3D().setWidth(texture->baseWidth).setHeight(texture->baseHeight).setDepth(1))
                .setQueueFamilyIndexCount(numIndices)
                .setPQueueFamilyIndices(indices.data())
                .setUsage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst)
                .setImageType(vk::ImageType::e2D)
                .setMipLevels(texture->numLevels)
                .setArrayLayers(1)
                .setTiling(vk::ImageTiling::eOptimal)
                .setInitialLayout(vk::ImageLayout::eUndefined)
                .setSamples(vk::SampleCountFlagBits::e1)
                .setSharingMode(vk::SharingMode::eConcurrent),
            "compresesd_texture")
        .setBaseFormat(static_cast<vk::Format>(texture->vkFormat))
        .addViewInfo(vk::ImageViewCreateInfo()
                         .setViewType(vk::ImageViewType::e2D)
                         .setFormat(static_cast<vk::Format>(texture->vkFormat))
                         .setSubresourceRange(vk::ImageSubresourceRange()
                                                  .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                  .setBaseArrayLayer(0)
                                                  .setLayerCount(1)
                                                  .setBaseMipLevel(0)
                                                  .setLevelCount(texture->numLevels)))
        .setSamplerInfo(vk::SamplerCreateInfo()
                            .setAnisotropyEnable(true)
                            .setMaxAnisotropy(StarTextures::Texture::SelectAnisotropyLevel(this->deviceProperties))
                            .setMagFilter(StarTextures::Texture::SelectTextureFiltering(this->deviceProperties))
                            .setMinFilter(StarTextures::Texture::SelectTextureFiltering(this->deviceProperties))
                            .setMinFilter(StarTextures::Texture::SelectTextureFiltering(this->deviceProperties))
                            .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
                            .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
                            .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
                            .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
                            .setUnnormalizedCoordinates(VK_FALSE)
                            .setCompareEnable(VK_FALSE)
                            .setCompareOp(vk::CompareOp::eAlways)
                            .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                            .setMipLodBias(0.0f)
                            .setMinLod(0.0f)
                            .setMaxLod(static_cast<float>(texture->numLevels)))
        .build();
}

void star::TransferRequest::CompressedTextureFile::copyFromTransferSRCToDST(StarBuffers::Buffer &srcBuffer,
                                                                            StarTextures::Texture &dst,
                                                                            vk::CommandBuffer &commandBuffer) const
{
    boost::unique_lock<boost::mutex> lock;
    ktxTexture2 *texture = nullptr;
    this->compressedTexture->giveMeTranscodedImage(lock, texture);

    {
        vk::ImageMemoryBarrier barrier{};
        barrier.sType = vk::StructureType::eImageMemoryBarrier;
        barrier.oldLayout = vk::ImageLayout::eUndefined;
        barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

        barrier.image = dst.getVulkanImage();
        barrier.srcAccessMask = vk::AccessFlagBits::eNone;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = texture->numLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, // which pipeline stages should
                                                                             // occurr before barrier
                                      vk::PipelineStageFlagBits::eTransfer,  // pipeline stage in
                                                                             // which operations will
                                                                             // wait on the barrier
                                      {}, {}, nullptr, barrier);
    }

    std::vector<vk::BufferImageCopy> bufferCopyRegions;

    for (int i = 0; i < texture->numLevels; i++)
    {
        ktx_size_t offset;
        if (ktxTexture_GetImageOffset((ktxTexture *)texture, i, 0, 0, &offset) != ktx_error_code_e::KTX_SUCCESS)
        {
            throw std::runtime_error("Failed to get image offset into compressed texture");
        }

        vk::BufferImageCopy copyRegion{};
        copyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        copyRegion.imageSubresource.mipLevel = i;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent.width = texture->baseWidth >> i;
        copyRegion.imageExtent.height = texture->baseHeight >> i;
        copyRegion.imageExtent.depth = 1;
        copyRegion.bufferOffset = offset;

        bufferCopyRegions.push_back(copyRegion);
    }

    commandBuffer.copyBufferToImage(srcBuffer.getVulkanBuffer(), dst.getVulkanImage(),
                                    vk::ImageLayout::eTransferDstOptimal, bufferCopyRegions);

    {
        vk::ImageMemoryBarrier barrier{};
        barrier.sType = vk::StructureType::eImageMemoryBarrier;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

        barrier.image = dst.getVulkanImage();
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eNone;

        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = texture->numLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,     // which pipeline stages should
                                                                                // occurr before barrier
                                      vk::PipelineStageFlagBits::eBottomOfPipe, // pipeline stage in
                                                                                // which operations will
                                                                                // wait on the barrier
                                      {}, {}, nullptr, barrier);
    }
}

void star::TransferRequest::CompressedTextureFile::writeDataToStageBuffer(StarBuffers::Buffer &buffer) const
{
    void *mapped = nullptr; 
    buffer.map(&mapped);

    {
        boost::unique_lock<boost::mutex> lock;
        ktxTexture2 *texture = nullptr;
        this->compressedTexture->giveMeTranscodedImage(lock, texture);

        buffer.writeToBuffer(texture->pData, mapped, texture->dataSize);
    }

    buffer.unmap();
}
