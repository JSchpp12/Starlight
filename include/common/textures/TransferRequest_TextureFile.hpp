#pragma once

#include "TransferRequest_Texture.hpp"

#include <string>

namespace star::TransferRequest{
    class TextureFile : public Texture{
        public:
        TextureFile(const std::string& imagePath, const vk::PhysicalDeviceProperties& deviceProperties);

        star::StarTexture::TextureCreateSettings getCreateArgs() const override;

        void writeData(star::StarBuffer& stagingBuffer) const override;

        virtual void copyFromTransferSRCToDST(StarBuffer& srcBuffer, StarTexture& dstTexture, vk::CommandBuffer& commandBuffer) const override; 

        protected:
        
        private:
        const std::string imagePath; 
        const vk::PhysicalDeviceProperties deviceProperties; 

        static void GetTextureInfo(const std::string& imagePath, int& width, int& height, int& channels);
    };
}