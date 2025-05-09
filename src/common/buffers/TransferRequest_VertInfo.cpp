#include "TransferRequest_VertInfo.hpp"

#include "CastHelpers.hpp"

std::unique_ptr<star::StarBuffer> star::TransferRequest::VertInfo::createFinal(vk::Device &device, VmaAllocator &allocator) const{
    auto create = StarBuffer::BufferCreationArgs{
        sizeof(Vertex), 
        CastHelpers::size_t_to_unsigned_int(this->vertices.size()),
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        VMA_MEMORY_USAGE_AUTO,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::SharingMode::eConcurrent,
        "VertInfoBuffer"
    };

    return std::make_unique<StarBuffer>(allocator, create); 
}

std::unique_ptr<star::StarBuffer> star::TransferRequest::VertInfo::createStagingBuffer(vk::Device &device, VmaAllocator &allocator) const{
    auto create = StarBuffer::BufferCreationArgs{
        sizeof(Vertex), 
        CastHelpers::size_t_to_unsigned_int(this->vertices.size()),
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        VMA_MEMORY_USAGE_AUTO,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::SharingMode::eConcurrent,
        "VertInfoBuffer"
    };

    return std::make_unique<StarBuffer>(allocator, create); 
}

void star::TransferRequest::VertInfo::writeDataToStageBuffer(StarBuffer &buffer) const{
    buffer.map(); 

    auto data = this->getVertices();

    for (int i = 0; i < data.size(); ++i){
        buffer.writeToIndex(&data.at(i), i);
    }

    buffer.unmap();
}

std::vector<star::Vertex> star::TransferRequest::VertInfo::getVertices() const{
    return this->vertices;
}
