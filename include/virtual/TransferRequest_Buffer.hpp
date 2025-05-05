#pragma once

#include "TransferRequest_Memory.hpp"

namespace star::TransferRequest{
    class Buffer : public Memory<StarBuffer::BufferCreationArgs>{
        public:
        Buffer() = default;
        ~Buffer() = default;
        
        virtual star::StarBuffer::BufferCreationArgs getCreateArgs() const override = 0;

        virtual void writeData(StarBuffer& buffer) const override = 0;

        virtual void copyFromTransferSRCToDST(StarBuffer& srcBuffer, StarBuffer& dstBuffer, vk::CommandBuffer& commandBuffer) const;

        protected:
        static void defaultCopy(StarBuffer& srcBuffer, StarBuffer& dstBuffer, vk::CommandBuffer& commandBuffer); 

        private:

    };
}