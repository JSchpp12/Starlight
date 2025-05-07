#pragma once

#include "TransferRequest_Texture.hpp"
#include "StarTexture.hpp"
#include "SharedCompressedTexture.hpp"

#include <ktx.h>

#include <vector>
#include <memory>

namespace star::TransferRequest{
    class CompressedTextureFile : public TransferRequest::Texture{
        public:
        CompressedTextureFile(const vk::PhysicalDeviceProperties& deviceProperties, std::shared_ptr<SharedCompressedTexture> compressedTexture, const uint8_t& mipMapIndex);

        StarTexture::RawTextureCreateSettings getCreateArgs() const override;
        
        void writeData(StarBuffer& buffer) const override; 

        void copyFromTransferSRCToDST(StarBuffer& srcBuffer, StarTexture& dstTexture, vk::CommandBuffer& commandBuffer) const override;

        private:
        std::shared_ptr<SharedCompressedTexture> compressedTexture = nullptr; 
        const uint8_t mipMapIndex;
        const vk::PhysicalDeviceProperties deviceProperties; 

        static void getTextureInfo(const std::string& imagePath, int& width, int& height, int& channels);
    };
}