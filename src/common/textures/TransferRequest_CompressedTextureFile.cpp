#include "TransferRequest_CompressedTextureFile.hpp"

#include "FileHelpers.hpp"

#include <assert.h>

star::TransferRequest::CompressedTextureFile::CompressedTextureFile(const vk::PhysicalDeviceProperties& deviceProperties, std::shared_ptr<SharedCompressedTexture> compressedTexture, const uint8_t& mipMapIndex) 
: compressedTexture(compressedTexture), mipMapIndex(mipMapIndex), deviceProperties(deviceProperties)
{
}

star::StarTexture::TextureCreateSettings star::TransferRequest::CompressedTextureFile::getCreateArgs() const{
    StarTexture::TextureCreateSettings createArgs;

    {
        boost::unique_lock<boost::mutex> lock;
        ktxTexture2* texture = nullptr; 
        this->compressedTexture->giveMeTranscodedImage(lock, texture); 

        createArgs.width = texture->baseWidth; 
        createArgs.height = texture->baseHeight; 
        createArgs.depth = texture->baseDepth; 
        createArgs.overrideImageMemorySize = texture->dataSize;
        createArgs.baseFormat = (vk::Format)texture->vkFormat;
        createArgs.anisotropyLevel = StarTexture::SelectAnisotropyLevel(this->deviceProperties); 
        createArgs.textureFilteringMode = StarTexture::SelectTextureFiltering(this->deviceProperties);
        createArgs.allocationName = star::FileHelpers::GetFileNameWithExtension(this->compressedTexture->getPathToFile());
        createArgs.createSampler = true; 
    }

    
    return createArgs;
}

void star::TransferRequest::CompressedTextureFile::writeData(StarBuffer& buffer) const{
    buffer.map();

    {
        boost::unique_lock<boost::mutex> lock;
        ktxTexture2* texture = nullptr;
        this->compressedTexture->giveMeTranscodedImage(lock, texture);

        buffer.writeToBuffer(texture->pData, texture->dataSize); 
    }

    buffer.unmap(); 
}

void star::TransferRequest::CompressedTextureFile::copyFromTransferSRCToDST(StarBuffer& srcBuffer, StarTexture& dstTexture, vk::CommandBuffer& commandBuffer) const{
    boost::unique_lock<boost::mutex> lock; 
    ktxTexture2* texture = nullptr; 
    this->compressedTexture->giveMeTranscodedImage(lock, texture);

    std::vector<vk::BufferImageCopy> bufferCopyRegions; 

    for (int i = 0; i < texture->numLevels; i++){
        ktx_size_t offset; 
        KTX_error_code result = ktxTexture_GetImageOffset((ktxTexture*)texture, i, 0, 0, &offset); 

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

    commandBuffer.copyBufferToImage(srcBuffer.getVulkanBuffer(), dstTexture.getImage(), vk::ImageLayout::eTransferDstOptimal, bufferCopyRegions); 
}