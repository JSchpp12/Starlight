#include "TransferRequest_TextureFile.hpp"

#include "Allocator.hpp"
#include "ConfigFile.hpp"
#include "Enums.hpp"
#include "FileHelpers.hpp"
#include "starlight/core/Exceptions.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <star_common/helper/CastHelpers.hpp>

#include <cassert>

static bool IsTextureFile(const std::string path)
{
    const std::filesystem::path fPath(path); 
    const auto ext = fPath.extension(); 
    if (ext == ".png" || ext == ".jpg")
    {
        return true; 
    }

    return false; 
}

star::TransferRequest::TextureFile::TextureFile(uint32_t graphicsQueueFamilyIndex,
                                                vk::PhysicalDeviceProperties deviceProperties, std::string imagePath)
    : graphicsQueueFamilyIndex(std::move(graphicsQueueFamilyIndex)), deviceProperties(std::move(deviceProperties)),
      m_imagePath(std::move(imagePath))
{
    if (!star::file_helpers::FileExists(m_imagePath) || !IsTextureFile(m_imagePath))
    {
        std::string msg = "Texture file does not exist or is not a valid image file:" + m_imagePath;
        STAR_THROW(msg);
    }
}

std::unique_ptr<star::StarBuffers::Buffer> star::TransferRequest::TextureFile::createStagingBuffer(
    vk::Device &device, VmaAllocator &allocator) const
{
    int width, height, channels = 0;
    GetTextureInfo(m_imagePath, width, height, channels);

    const vk::DeviceSize size = width * height * channels * 4;

    return StarBuffers::Buffer::Builder(allocator)
        .setAllocationCreateInfo(
            Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eExclusive)
                .setSize(size)
                .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
            m_imagePath + "_TransferSRCBuffer")
        .setInstanceCount(1)
        .setInstanceSize(size)
        .buildUnique();
}

std::unique_ptr<star::StarTextures::Texture> star::TransferRequest::TextureFile::createFinal(
    vk::Device &device, VmaAllocator &allocator, const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    int width, height, channels = 0;
    GetTextureInfo(m_imagePath, width, height, channels);

    std::vector<uint32_t> indices = std::vector<uint32_t>{this->graphicsQueueFamilyIndex};
    for (auto &index : transferQueueFamilyIndex)
    {
        indices.push_back(index);
    }

    return star::StarTextures::Texture::Builder(device, allocator)
        .setCreateInfo(Allocator::AllocationBuilder()
                           .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                           .setUsage(VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO)
                           .build(),
                       vk::ImageCreateInfo()
                           .setExtent(vk::Extent3D().setWidth(width).setHeight(height).setDepth(1))
                           .setUsage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst)
                           .setImageType(vk::ImageType::e2D)
                           .setMipLevels(1)
                           .setArrayLayers(1)
                           .setTiling(vk::ImageTiling::eOptimal)
                           .setInitialLayout(vk::ImageLayout::eUndefined)
                           .setSamples(vk::SampleCountFlagBits::e1)
                           .setSharingMode(vk::SharingMode::eConcurrent)
                           .setPQueueFamilyIndices(indices.data())
                           .setQueueFamilyIndexCount(indices.size()),
                       m_imagePath)
        .setBaseFormat(vk::Format::eR8G8B8A8Srgb)
        .addViewInfo(vk::ImageViewCreateInfo()
                         .setViewType(vk::ImageViewType::e2D)
                         .setFormat(vk::Format::eR8G8B8A8Srgb)
                         .setSubresourceRange(vk::ImageSubresourceRange()
                                                  .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                  .setBaseArrayLayer(0)
                                                  .setLayerCount(1)
                                                  .setBaseMipLevel(0)
                                                  .setLevelCount(1)))
        .setSamplerInfo(vk::SamplerCreateInfo()
                            .setAnisotropyEnable(true)
                            .setMaxAnisotropy(StarTextures::Texture::SelectAnisotropyLevel(this->deviceProperties))
                            .setMagFilter(StarTextures::Texture::SelectTextureFiltering(this->deviceProperties))
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
                            .setMaxLod(0.0f))
        .buildUnique();
}

void star::TransferRequest::TextureFile::writeDataToStageBuffer(star::StarBuffers::Buffer &stagingBuffer) const
{
    int l_width, l_height, l_channels = 0;
    unsigned char *pixelData(stbi_load(m_imagePath.c_str(), &l_width, &l_height, &l_channels, STBI_rgb_alpha));

    if (!star::file_helpers::FileExists(m_imagePath))
    {
        throw std::runtime_error("Provided file does not exist");
    }

    if (!pixelData)
    {
        throw std::runtime_error("Unable to load image");
    }

    // apply overriden alpha value if needed

    for (int i = 0; i < l_height; i++)
    {
        for (int j = 0; j < l_width; j++)
        {
            unsigned char *pixelOffset = pixelData + (i + l_height * j) * 4;
            pixelOffset[3] = (unsigned char)255;
        }
    }

    const vk::DeviceSize imageSize = l_width * l_height * 4;

    void *mapped = nullptr;
    stagingBuffer.map(&mapped);
    stagingBuffer.writeToBuffer(pixelData, mapped, imageSize);
    stagingBuffer.unmap();

    stbi_image_free(pixelData);
}

void star::TransferRequest::TextureFile::copyFromTransferSRCToDST(star::StarBuffers::Buffer &srcBuffer,
                                                                  star::StarTextures::Texture &dstTexture,
                                                                  vk::CommandBuffer &commandBuffer) const
{

    // transfer to transferdst
    StarTextures::Texture::TransitionImageLayout(dstTexture, commandBuffer, dstTexture.getBaseFormat(),
                                                 vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

    uint32_t width, height, channels;
    {
        int rWidth, rHeight, rChannels;
        GetTextureInfo(m_imagePath, rWidth, rHeight, rChannels);
        if (!common::helper::SafeCast<int, uint32_t>(rWidth, width) ||
            !common::helper::SafeCast<int, uint32_t>(rHeight, height) ||
            !common::helper::SafeCast<int, uint32_t>(rChannels, channels))
        {
            throw std::runtime_error("Invalid values read from texture");
        }
    }

    vk::BufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = vk::Offset3D{};
    region.imageExtent = vk::Extent3D{width, height, 1};

    commandBuffer.copyBufferToImage(srcBuffer.getVulkanBuffer(), dstTexture.getVulkanImage(),
                                    vk::ImageLayout::eTransferDstOptimal, region);

    StarTextures::Texture::TransitionImageLayout(dstTexture, commandBuffer, dstTexture.getBaseFormat(),
                                                 vk::ImageLayout::eTransferDstOptimal,
                                                 vk::ImageLayout::eShaderReadOnlyOptimal);
}

void star::TransferRequest::TextureFile::GetTextureInfo(const std::string &imagePath, int &width, int &height,
                                                        int &channels)
{
    stbi_info(imagePath.c_str(), &width, &height, &channels);
}
