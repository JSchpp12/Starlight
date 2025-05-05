#include "ManagerController_RenderResource_GlobalInfo.hpp"

std::vector<std::unique_ptr<star::TransferRequest::Memory<star::StarBuffer::BufferCreationArgs>>> star::ManagerController::RenderResource::GlobalInfo::createTransferRequests(const vk::PhysicalDevice& physicalDevice) {
    std::vector<std::unique_ptr<star::TransferRequest::Memory<star::StarBuffer::BufferCreationArgs>>> result; 
    
    this->lastNumLights = this->numLights; 

    result.emplace_back(std::make_unique<TransferRequest::GlobalInfo>(this->camera, this->numLights));

    return result;
}