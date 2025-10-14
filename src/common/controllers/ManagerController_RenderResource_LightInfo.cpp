#include "ManagerController_RenderResource_LightInfo.hpp"

#include "TransferRequest_LightInfo.hpp"

std::unique_ptr<star::TransferRequest::Buffer> star::ManagerController::RenderResource::LightInfo::createTransferRequest(star::core::device::StarDevice &device, const uint8_t &frameInFlightIndex){
    uint32_t numLights; 
    star::CastHelpers::SafeCast<size_t, uint32_t>(this->lights.size(), numLights); 

    this->lastWriteNumLights[frameInFlightIndex] = numLights; 
    
    return std::make_unique<TransferRequest::LightInfo>(
        numLights,
        device.getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex()
    ); 
}

bool star::ManagerController::RenderResource::LightInfo::isValid(const uint8_t& currentFrameInFlightIndex) const{
    assert(currentFrameInFlightIndex < lastWriteNumLights.size() && "Not enough resources were created for this"); 

    if (!star::ManagerController::RenderResource::Buffer::isValid(currentFrameInFlightIndex) && this->lastWriteNumLights[currentFrameInFlightIndex] != this->lights.size()){
		return false;
	}

	return true;
}