#include "ManagerController_RenderResource_VertInfo.hpp"

#include "TransferRequest_VertInfo.hpp"

std::unique_ptr<star::TransferRequest::Memory<star::StarBuffer::BufferCreationArgs>> star::ManagerController::RenderResource::VertInfo::createTransferRequest(){
    return std::make_unique<TransferRequest::VertInfo>(this->vertices);
}
