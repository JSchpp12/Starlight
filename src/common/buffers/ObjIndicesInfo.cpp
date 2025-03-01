#include "ObjIndicesInfo.hpp"

void star::ObjIndicesTransfer::writeData(star::StarBuffer& buffer) const {

    buffer.map();
    
    vk::DeviceSize indSize = sizeof(uint32_t) * this->indices.size();
    std::vector<uint32_t> cpInd{this->indices};
    buffer.writeToBuffer(cpInd.data(), indSize);

    buffer.unmap();
}

std::unique_ptr<star::BufferMemoryTransferRequest> star::ObjIndicesInfo::createTransferRequest() const{
    return std::make_unique<ObjIndicesTransfer>(this->indices);
}
