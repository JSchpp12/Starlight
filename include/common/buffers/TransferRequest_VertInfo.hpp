#pragma once

#include "TransferRequest_Memory.hpp"

#include "Vertex.hpp"

namespace star::TransferRequest{
    class VertInfo : public Memory<StarBuffer::BufferCreationArgs>{
        public:
        VertInfo(std::vector<Vertex> vertices) 
        : vertices(vertices){}

        StarBuffer::BufferCreationArgs getCreateArgs(const vk::PhysicalDeviceProperties& deviceProperties) const override;

        void writeData(StarBuffer& buffer) const override; 

        protected:
        std::vector<Vertex> vertices;

        virtual std::vector<Vertex> getVertices() const; 
    };
}