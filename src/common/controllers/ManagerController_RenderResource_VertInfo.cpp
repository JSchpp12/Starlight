#include "ManagerController_RenderResource_VertInfo.hpp"

#include "TransferRequest_VertInfo.hpp"

std::vector<std::unique_ptr<star::TransferRequest::Memory<star::StarBuffer::BufferCreationArgs>>> star::ManagerController::RenderResource::VertInfo::createTransferRequests(const vk::PhysicalDevice& physicalDevice){
    std::vector<std::unique_ptr<star::TransferRequest::Memory<star::StarBuffer::BufferCreationArgs>>> result; 

    result.emplace_back(std::make_unique<TransferRequest::VertInfo>(this->vertices));

    return result;
}
