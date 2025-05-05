#pragma once 

#include "TransferRequest_Buffer.hpp"

#include <vector>

namespace star::TransferRequest{
    class IndicesInfo : public Buffer {
        public:
        IndicesInfo(const std::vector<uint32_t>& indices) : indices(indices){}

        void writeData(StarBuffer& buffer) const override;

        StarBuffer::BufferCreationArgs getCreateArgs() const override;

        void copyFromTransferSRCToDST(StarBuffer& srcBuffer, StarBuffer& dstBuffer, vk::CommandBuffer& commandBuffer) const override; 

        protected:
        const std::vector<uint32_t> indices;
        
    };
}