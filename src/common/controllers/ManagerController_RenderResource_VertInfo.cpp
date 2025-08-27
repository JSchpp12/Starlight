#include "ManagerController_RenderResource_VertInfo.hpp"

#include "TransferRequest_VertInfo.hpp"

std::unique_ptr<star::TransferRequest::Buffer> star::ManagerController::RenderResource::VertInfo::createTransferRequest(
    star::core::device::StarDevice &device)
{
    return std::make_unique<TransferRequest::VertInfo>(
        device.getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex(), this->vertices);
}
