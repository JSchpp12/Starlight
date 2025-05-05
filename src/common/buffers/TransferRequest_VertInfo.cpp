#include "TransferRequest_VertInfo.hpp"

#include "CastHelpers.hpp"

star::StarBuffer::BufferCreationArgs star::TransferRequest::VertInfo::getCreateArgs() const{
    return StarBuffer::BufferCreationArgs{
        sizeof(Vertex), 
        CastHelpers::size_t_to_unsigned_int(this->vertices.size()),
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        VMA_MEMORY_USAGE_AUTO,
        vk::BufferUsageFlagBits::eVertexBuffer,
        vk::SharingMode::eConcurrent,
        "VertInfoBuffer"
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

void star::TransferRequest::VertInfo::copyFromTransferSRCToDST(StarBuffer &srcBuffer, StarBuffer &dstBuffer, vk::CommandBuffer &commandBuffer) const{
    vk::BufferCopy copyRegion{}; 
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = srcBuffer.getBufferSize(); 

    commandBuffer.copyBuffer(srcBuffer.getVulkanBuffer(), dstBuffer.getVulkanBuffer(), copyRegion);
}


std::vector<star::Vertex> star::TransferRequest::VertInfo::getVertices() const{
    return this->vertices;
}
