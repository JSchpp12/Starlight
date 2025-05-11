#pragma once

#include "TransferRequest_Buffer.hpp"

#include "Vertex.hpp"

namespace star::TransferRequest{
    class VertInfo : public Buffer{
        public:
        VertInfo(std::vector<Vertex> vertices) 
        : vertices(vertices){}

        std::unique_ptr<StarBuffer> createStagingBuffer(vk::Device& device, VmaAllocator& allocator, const uint32_t& transferQueueFamilyIndex) const override; 

        std::unique_ptr<StarBuffer> createFinal(vk::Device &device, VmaAllocator &allocator, const uint32_t& transferQueueFamilyIndex) const override; 
        
        void writeDataToStageBuffer(StarBuffer& buffer) const override; 
        protected:
        std::vector<Vertex> vertices;

        virtual std::vector<Vertex> getVertices() const; 
    };
}