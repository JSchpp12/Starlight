#include "ManagerController_RenderResource_GlobalInfo.hpp"

#include "TransferRequest_GlobalInfo.hpp"

std::unique_ptr<star::TransferRequest::Buffer> star::ManagerController::RenderResource::GlobalInfo::createTransferRequest(star::core::device::StarDevice &device) {
    return std::make_unique<TransferRequest::GlobalInfo>(
        *this->camera, 
        device.getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex());
}