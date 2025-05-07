#include "TransferRequest_TextureFile.hpp"

#include "Enums.hpp"
#include "FileHelpers.hpp"
#include "ConfigFile.hpp"
#include "CastHelpers.hpp"

#include "stb_image.h"

#include <assert.h>

star::TransferRequest::TextureFile::TextureFile(const std::string& imagePath, const vk::PhysicalDeviceProperties& deviceProperties) : imagePath(imagePath), deviceProperties(deviceProperties){
    assert(star::FileHelpers::FileExists(this->imagePath) && "Provided path does not exist");
}

star::StarTexture::RawTextureCreateSettings star::TransferRequest::TextureFile::getCreateArgs() const{
    int width, height, channels = 0;
    GetTextureInfo(this->imagePath, width, height, channels);

    auto iSet = star::StarTexture::RawTextureCreateSettings{};
    iSet.width = width;
    iSet.height = height; 
    iSet.channels = 4;      //overriding the channels to 4 for simplicity
    iSet.byteDepth = 1;
    iSet.depth = 1;
    iSet.usage = vk::ImageUsageFlagBits::eSampled;
    iSet.baseFormat = vk::Format::eR8G8B8A8Srgb;
    iSet.aspectFlags = vk::ImageAspectFlagBits::eColor;
    iSet.memoryUsage = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO;
    iSet.allocationCreateFlags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    iSet.initialLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    iSet.isMutable = false; 
    iSet.createSampler = true; 
    iSet.anisotropyLevel = StarTexture::SelectAnisotropyLevel(this->deviceProperties);
    iSet.textureFilteringMode = StarTexture::SelectTextureFiltering(this->deviceProperties);
    iSet.allocationName = this->imagePath; 
    return iSet;
}

void star::TransferRequest::TextureFile::writeData(star::StarBuffer& stagingBuffer) const{
    int l_width, l_height, l_channels = 0;
    unsigned char* pixelData(stbi_load(this->imagePath.c_str(), &l_width, &l_height, &l_channels, STBI_rgb_alpha));

    if (!pixelData) {
        throw std::runtime_error("Unable to load image");
    }

    //apply overriden alpha value if needed

    for (int i = 0; i < l_height; i++) {
        for (int j = 0; j < l_width; j++) {
            unsigned char* pixelOffset = pixelData + (i + l_height * j) * 4;
            pixelOffset[3] = (unsigned char)255;
        }
    }

    vk::DeviceSize imageSize = l_width * l_height * 4; 

    stagingBuffer.map();
    stagingBuffer.writeToBuffer(pixelData, imageSize);
    stagingBuffer.unmap();

    stbi_image_free(pixelData);
}

void star::TransferRequest::TextureFile::copyFromTransferSRCToDST(star::StarBuffer& srcBuffer, star::StarTexture& dstTexture, vk::CommandBuffer& commandBuffer) const{

    int width, height, channels; 
    GetTextureInfo(this->imagePath, width, height, channels); 

    vk::BufferImageCopy region{}; 
    region.bufferOffset = 0; 
    region.bufferRowLength = 0; 
    region.bufferImageHeight = 0; 

    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor; 
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = vk::Offset3D{}; 
    region.imageExtent = vk::Extent3D{
        CastHelpers::int_to_unsigned_int(width),
        CastHelpers::int_to_unsigned_int(height), 
        1
    };

    commandBuffer.copyBufferToImage(srcBuffer.getVulkanBuffer(), dstTexture.getImage(), vk::ImageLayout::eTransferDstOptimal, region);
}

void star::TransferRequest::TextureFile::GetTextureInfo(const std::string& imagePath, int& width, int& height, int& channels){
    stbi_info(imagePath.c_str(), &width, &height, &channels);
}
