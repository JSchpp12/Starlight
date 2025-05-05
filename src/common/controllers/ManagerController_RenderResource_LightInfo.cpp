#include "ManagerController_RenderResource_LightInfo.hpp"

#include "TransferRequest_LightInfo.hpp"

std::vector<std::unique_ptr<star::TransferRequest::Memory<star::StarBuffer::BufferCreationArgs>>> star::ManagerController::RenderResource::LightInfo::createTransferRequests(const vk::PhysicalDevice& physicalDevice){
    this->lastWriteNumLights = this->lights.size();

    auto result = std::vector<std::unique_ptr<star::TransferRequest::Memory<star::StarBuffer::BufferCreationArgs>>>(); 
    result.emplace_back(std::make_unique<TransferRequest::LightInfo>(this->lights)); 

    return result;
}

bool star::ManagerController::RenderResource::LightInfo::isValid(const uint8_t& currentFrameInFlightIndex) const{
    if (!star::ManagerController::RenderResource::Buffer::isValid(currentFrameInFlightIndex) && this->lastWriteNumLights != this->lights.size()){
		return false;
	}

	return true;
}