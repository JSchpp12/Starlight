#pragma once 

#include "TransferRequest_Memory.hpp"

#include <vector>

namespace star::TransferRequest{
    class IndicesInfo : public Memory<StarBuffer::BufferCreationArgs> {
        public:
        IndicesInfo(const std::vector<uint32_t>& indices) : indices(indices){}

        void writeData(StarBuffer& buffer) const override;

        StarBuffer::BufferCreationArgs getCreateArgs(const vk::PhysicalDeviceProperties& deviceProperties) const override;

        protected:
        const std::vector<uint32_t> indices;
    };
}