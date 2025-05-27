#include "ManagerController_RenderResource_LightInfo.hpp"

#include "TransferRequest_LightInfo.hpp"

std::unique_ptr<star::TransferRequest::Buffer> star::ManagerController::RenderResource::LightInfo::createTransferRequest(star::StarDevice &device){
    this->lastWriteNumLights = this->lights.size();
 
    return std::make_unique<TransferRequest::LightInfo>(
        this->lights,
        device.getQueueFamily(star::Queue_Type::Tgraphics).getQueueFamilyIndex()
    ); 
}

bool star::ManagerController::RenderResource::LightInfo::isValid(const uint8_t& currentFrameInFlightIndex) const{
    if (!star::ManagerController::RenderResource::Buffer::isValid(currentFrameInFlightIndex) && this->lastWriteNumLights != this->lights.size()){
		return false;
	}

	return true;
}