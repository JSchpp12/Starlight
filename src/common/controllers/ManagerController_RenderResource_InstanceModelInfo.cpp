#include "ManagerController_RenderResource_InstanceModelInfo.hpp"

#include "TransferRequest_InstanceModelInfo.hpp"

std::unique_ptr<star::TransferRequest::Buffer> star::ManagerController::RenderResource::InstanceModelInfo::createTransferRequest(const vk::PhysicalDevice& physicalDevice){
    return std::make_unique<TransferRequest::InstanceModelInfo>(this->objectInstances);
}