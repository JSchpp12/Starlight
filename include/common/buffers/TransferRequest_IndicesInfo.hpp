#pragma once 

#include "TransferRequest_Buffer.hpp"

#include <vector>

namespace star::TransferRequest{
    class IndicesInfo : public Buffer {
        public:
        IndicesInfo(const std::vector<uint32_t>& indices) : indices(indices){}

        std::unique_ptr<StarBuffer> createStagingBuffer(vk::Device& device, VmaAllocator& allocator) const override; 

        std::unique_ptr<StarBuffer> createFinal(vk::Device &device, VmaAllocator &allocator) const override; 
        
        void writeDataToStageBuffer(StarBuffer& buffer) const override; 
        protected:
        const std::vector<uint32_t> indices;
        
    };
}