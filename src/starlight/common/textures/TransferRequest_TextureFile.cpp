#include "TransferRequest_TextureFile.hpp"

#include "Allocator.hpp"
#include "ConfigFile.hpp"
#include "FileHelpers.hpp"
#include "starlight/core/Exceptions.hpp"
#include "starlight/maker/buffer/MakeBuffer.hpp"
#include "starlight/maker/texture/MakeTexture.hpp"
#include "starlight/structs/ImageFileInfo.hpp"
#include "starlight/util/io/ReadPng.hpp"

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

static star::ImageFileInfo GetImageInfoOrThrow(const std::string &path)
{
    const auto imageInfo = star::io::ReadPng(path).getInfo();
    if (!imageInfo)
    {
        STAR_THROW(imageInfo.error().what());
    }

    return *imageInfo;
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

    m_imageInfo = GetImageInfoOrThrow(m_imagePath);
}

std::string star::TransferRequest::TextureFile::getDebugDescription() const
{
    return m_imagePath;
}

std::unique_ptr<star::StarBuffers::Buffer> star::TransferRequest::TextureFile::createStagingBuffer(
    core::device::StarDevice &device) const
{
    const auto &imageInfo = m_imageInfo;

    // stb expands every image to RGBA when loading (STBI_rgb_alpha), so the staging buffer
    // must hold four bytes for every pixel regardless of the file's channel format.
    const vk::DeviceSize size =
        static_cast<vk::DeviceSize>(imageInfo.resolution.x) * static_cast<vk::DeviceSize>(imageInfo.resolution.y) *
        static_cast<vk::DeviceSize>(imageInfo.resolution.z) * GetSizeInMemory(ImageChannelFormat::Rgba);

    star::StarBuffers::Buffer builtBuffer;
    const auto makeResult = star::maker::buffer::MakeBuffer(device)
                                .setCreateInfo(Allocator::AllocationBuilder()
                                                   .setFlags(VMA_ALLOCATION_CREATE_MAPPED_BIT |
                                                             VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT)
                                                   .setUsage(VMA_MEMORY_USAGE_AUTO)
                                                   .build(),
                                               vk::BufferCreateInfo()
                                                   .setSharingMode(vk::SharingMode::eExclusive)
                                                   .setSize(size)
                                                   .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
                                               m_imagePath + "_TransferSRCBuffer")
                                .setInstanceCount(1)
                                .setInstanceSize(size)
                                .make(&builtBuffer, 1);
    if (!makeResult)
    {
        STAR_THROW("Failed to create staging buffer for texture file: " + makeResult.error().detail);
    }

    return std::make_unique<star::StarBuffers::Buffer>(std::move(builtBuffer));
}

std::unique_ptr<star::StarTextures::Texture> star::TransferRequest::TextureFile::createFinal(
    core::device::StarDevice &device, const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    const auto &imageInfo = m_imageInfo;
    const auto width = static_cast<uint32_t>(imageInfo.resolution.x);
    const auto height = static_cast<uint32_t>(imageInfo.resolution.y);

    std::vector<uint32_t> indices = std::vector<uint32_t>{this->graphicsQueueFamilyIndex};
    indices.reserve(transferQueueFamilyIndex.size() + 1);
    for (auto &index : transferQueueFamilyIndex)
        indices.push_back(index);

    star::StarTextures::Texture texture;
    const auto makeResult =
        star::maker::texture::MakeTexture(device)
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
                               .setQueueFamilyIndexCount(static_cast<uint32_t>(indices.size())),
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
            .make(&texture, 1);
    if (!makeResult)
    {
        STAR_THROW("Failed to create texture from file: " + makeResult.error().detail);
    }

    return std::make_unique<star::StarTextures::Texture>(std::move(texture));
}

void star::TransferRequest::TextureFile::writeDataToStageBuffer(star::StarBuffers::Buffer &stagingBuffer) const
{
    const io::ReadPng reader(m_imagePath);

    auto pngData = reader.read();
    if (!pngData)
    {
        STAR_THROW(pngData.error().what());
    }

    const auto pixelData = pngData->data();

    void *mapped = nullptr;
    stagingBuffer.map(&mapped);
    stagingBuffer.writeToBuffer(pixelData.data(), mapped, pixelData.size());
    stagingBuffer.unmap();
}

void star::TransferRequest::TextureFile::copyFromTransferSRCToDST(star::StarBuffers::Buffer &srcBuffer,
                                                                  star::StarTextures::Texture &dstTexture,
                                                                  vk::CommandBuffer &commandBuffer) const
{

    // transfer to transferdst
    StarTextures::Texture::TransitionImageLayout(dstTexture, commandBuffer, dstTexture.getBaseFormat(),
                                                 vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

    const auto &imageInfo = m_imageInfo;
    const auto width = static_cast<uint32_t>(imageInfo.resolution.x);
    const auto height = static_cast<uint32_t>(imageInfo.resolution.y);

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
