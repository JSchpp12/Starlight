#include "ObjIndicesInfo.hpp"

void star::TransferRequest::ObjIndicies::writeData(star::StarBuffer& buffer) const {

    buffer.map();
    
    vk::DeviceSize indSize = sizeof(uint32_t) * this->indices.size();
    std::vector<uint32_t> cpInd{this->indices};
    buffer.writeToBuffer(cpInd.data(), indSize);

    buffer.unmap();
}

std::unique_ptr<star::TransferRequest::Memory<star::StarBuffer::BufferCreationArgs>> star::ObjIndicesInfo::createTransferRequest() const{
    return std::make_unique<TransferRequest::ObjIndicies>(this->indices);
}
