#pragma once

#include "TransferRequest_Memory.hpp"
#include "StarTexture.hpp"

#include <vulkan/vulkan.hpp>

namespace star::TransferRequest{
    class Texture : private Memory<star::StarTexture::TextureCreateSettings>{
        public:
        Texture() = default; 
        ~Texture() = default; 

        virtual StarTexture::TextureCreateSettings getCreateArgs() const override = 0; 

        virtual void writeData(star::StarBuffer& stagingBuffer) const override = 0; 

        virtual void copyFromTransferSRCToDST(StarBuffer& srcBuffer, StarTexture& dstTexture, vk::CommandBuffer& commandBuffer) const = 0; 

        protected:

    };
}