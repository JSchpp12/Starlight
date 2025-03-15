#include "ManagerController_RenderResource_GlobalInfo.hpp"

std::unique_ptr<star::TransferRequest::Memory<star::StarBuffer::BufferCreationArgs>> star::ManagerController::RenderResource::GlobalInfo::createTransferRequest() {
    this->lastNumLights = this->numLights; 
    return std::make_unique<TransferRequest::GlobalInfo>(this->camera, this->numLights);
}