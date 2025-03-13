#include "TransferRequest_VertInfo.hpp"

#include "CastHelpers.hpp"

star::StarBuffer::BufferCreationArgs star::TransferRequest::VertInfo::getCreateArgs(const vk::PhysicalDeviceProperties& deviceProperties) const{
    return StarBuffer::BufferCreationArgs{
        sizeof(Vertex), 
        CastHelpers::size_t_to_unsigned_int(this->vertices.size()),
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        VMA_MEMORY_USAGE_AUTO,
        vk::BufferUsageFlagBits::eVertexBuffer,
        vk::SharingMode::eConcurrent
    };
}

void star::TransferRequest::VertInfo::writeData(StarBuffer &buffer) const{
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
