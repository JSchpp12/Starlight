#pragma once

#include "TransferRequest_Buffer.hpp"

#include "Vertex.hpp"

namespace star::TransferRequest{
    class VertInfo : public Buffer{
        public:
        VertInfo(std::vector<Vertex> vertices) 
        : vertices(vertices){}

        StarBuffer::BufferCreationArgs getCreateArgs() const override;

        void writeData(StarBuffer& buffer) const override; 

        void copyFromTransferSRCToDST(StarBuffer& srcBuffer, StarBuffer& dstBuffer, vk::CommandBuffer& commandBuffer) const override; 

        protected:
        std::vector<Vertex> vertices;

        virtual std::vector<Vertex> getVertices() const; 
    };
}