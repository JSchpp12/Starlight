#include "ManagerController_RenderResource_IndicesInfo.hpp"

#include "TransferRequest_IndicesInfo.hpp"

std::unique_ptr<star::TransferRequest::Memory<star::StarBuffer::BufferCreationArgs>> star::ManagerController::RenderResource::IndicesInfo::createTransferRequest(){
    return std::make_unique<TransferRequest::IndicesInfo>(this->indices);
}