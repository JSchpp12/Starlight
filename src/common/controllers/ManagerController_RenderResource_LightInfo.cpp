#include "ManagerController_RenderResource_LightInfo.hpp"

#include "TransferRequest_LightInfo.hpp"

std::unique_ptr<star::TransferRequest::Memory<star::StarBuffer::BufferCreationArgs>> star::ManagerController::RenderResource::LightInfo::createTransferRequest(){
    this->lastWriteNumLights = this->lights.size();

    return std::make_unique<TransferRequest::LightInfo>(this->lights);
}

bool star::ManagerController::RenderResource::LightInfo::isValid(const uint8_t& currentFrameInFlightIndex) const{
    if (!star::ManagerController::RenderResource::Buffer::isValid(currentFrameInFlightIndex) && this->lastWriteNumLights != this->lights.size()){
		return false;
	}

	return true;
}