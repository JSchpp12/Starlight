#include "ManagerController_RenderResource_InstanceModelInfo.hpp"

#include "TransferRequest_InstanceModelInfo.hpp"

std::unique_ptr<star::TransferRequest::Memory<star::StarBuffer::BufferCreationArgs>> star::ManagerController::RenderResource::InstanceModelInfo::createTransferRequest(){
    return std::make_unique<TransferRequest::InstanceModelInfo>(this->objectInstances);
}