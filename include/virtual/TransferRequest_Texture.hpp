#pragma once

#include "TransferRequest_Memory.hpp"
#include "StarTexture.hpp"

#include <vulkan/vulkan.hpp>

namespace star::TransferRequest{
    class Texture : private Memory<star::StarTexture>{
        public:
        Texture() = default; 
        ~Texture() = default; 

        virtual std::unique_ptr<StarBuffer> createStagingBuffer(vk::Device& device, VmaAllocator& allocator) const override = 0; 

        virtual std::unique_ptr<star::StarTexture> createFinal(vk::Device& device, VmaAllocator& allocator) const override = 0; 

        virtual void copyFromTransferSRCToDST(StarBuffer& srcBuffer, StarTexture& dst, vk::CommandBuffer& commandBuffer) const = 0; 

        virtual void writeDataToStageBuffer(star::StarBuffer& stagingBuffer) const override = 0; 

        protected:

    };
}