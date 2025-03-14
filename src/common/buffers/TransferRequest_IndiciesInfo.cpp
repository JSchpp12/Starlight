#include "TransferRequest_IndicesInfo.hpp"

#include "CastHelpers.hpp"


void star::TransferRequest::IndicesInfo::writeData(StarBuffer& buffer) const{
    buffer.map();
    
    vk::DeviceSize indSize = sizeof(uint32_t) * this->indices.size();
    std::vector<uint32_t> cpInd{this->indices};
    buffer.writeToBuffer(cpInd.data(), indSize);

    buffer.unmap();
}

star::StarBuffer::BufferCreationArgs star::TransferRequest::IndicesInfo::getCreateArgs(const vk::PhysicalDeviceProperties& deviceProperties) const{
    return StarBuffer::BufferCreationArgs{
        sizeof(uint32_t),
        CastHelpers::size_t_to_unsigned_int(this->indices.size()),
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        VMA_MEMORY_USAGE_AUTO, 
        vk::BufferUsageFlagBits::eIndexBuffer, 
        vk::SharingMode::eConcurrent,
        "IndiciesInfoBuffer"
    };
}