#include "ManagerController_RenderResource_IndicesInfo.hpp"

#include "TransferRequest_IndicesInfo.hpp"

std::vector<std::unique_ptr<star::TransferRequest::Memory<star::StarBuffer::BufferCreationArgs>>> star::ManagerController::RenderResource::IndicesInfo::createTransferRequests(const vk::PhysicalDevice& physicalDevice){
    auto result = std::vector<std::unique_ptr<star::TransferRequest::Memory<star::StarBuffer::BufferCreationArgs>>>();

    result.emplace_back(std::make_unique<TransferRequest::IndicesInfo>(this->indices));
    
    return result;
}