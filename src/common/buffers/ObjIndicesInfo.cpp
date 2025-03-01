#include "ObjIndicesInfo.hpp"

void star::ObjIndicesTransfer::writeData(star::StarBuffer& buffer) const {

    buffer.map();

    for (int i = 0; i < indices.size(); ++i){
        uint32_t index = indices.at(i);
        buffer.writeToIndex(&index, sizeof(uint32_t));
    }

    buffer.unmap();
}

std::unique_ptr<star::BufferMemoryTransferRequest> star::ObjIndicesInfo::createTransferRequest() const{
    return std::make_unique<ObjIndicesTransfer>(this->indices);
}
