#pragma once

#include "TransferRequest_Memory.hpp"

namespace star::TransferRequest{
    class Buffer : public Memory<StarBuffer>{
        public:
        Buffer() = default;
        ~Buffer() = default;
        
        virtual std::unique_ptr<StarBuffer> createStagingBuffer(vk::Device& device, VmaAllocator& allocator) const override = 0; 

        virtual std::unique_ptr<StarBuffer> createFinal(vk::Device& device, VmaAllocator& allocator) const override = 0; 

        virtual void copyFromTransferSRCToDST(StarBuffer &srcBuffer, StarBuffer &dst, vk::CommandBuffer &commandBuffer) const override; 

        virtual void writeDataToStageBuffer(StarBuffer& buffer) const override = 0; 

        protected:
        static void defaultCopy(StarBuffer& srcBuffer, StarBuffer& dstBuffer, vk::CommandBuffer& commandBuffer); 

        private:

    };
}