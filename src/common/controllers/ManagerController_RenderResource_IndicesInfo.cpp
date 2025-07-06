#include "ManagerController_RenderResource_IndicesInfo.hpp"

#include "TransferRequest_IndicesInfo.hpp"

std::unique_ptr<star::TransferRequest::Buffer> star::ManagerController::RenderResource::IndicesInfo::createTransferRequest(StarDevice &device){
    return std::make_unique<TransferRequest::IndicesInfo>(this->indices, device.getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex());
}
