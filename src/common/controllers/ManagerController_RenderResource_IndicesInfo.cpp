#include "ManagerController_RenderResource_IndicesInfo.hpp"

#include "TransferRequest_IndicesInfo.hpp"

std::unique_ptr<star::TransferRequest::Buffer> star::ManagerController::RenderResource::IndicesInfo::createTransferRequest(const vk::PhysicalDevice& physicalDevice){
    return std::make_unique<TransferRequest::IndicesInfo>(this->indices);
}
