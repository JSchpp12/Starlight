#include "ManagerController_RenderResource_GlobalInfo.hpp"


bool star::ManagerController::RenderResource::GlobalInfo::isValid(const uint8_t& currentFrameInFlightIndex) const{
    if (this->getFrameInFlightIndexToUpdateOn() == currentFrameInFlightIndex && this->numLights != lastNumLights)
        return false;
    return true;
}

std::unique_ptr<star::TransferRequest::Memory<star::StarBuffer::BufferCreationArgs>> star::ManagerController::RenderResource::GlobalInfo::createTransferRequest() {
    this->lastNumLights = this->numLights; 

    return std::make_unique<TransferRequest::GlobalInfo>(this->camera, this->numLights);
}