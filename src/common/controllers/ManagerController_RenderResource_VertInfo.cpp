#include "ManagerController_RenderResource_VertInfo.hpp"

#include "TransferRequest_VertInfo.hpp"

std::unique_ptr<star::TransferRequest::Buffer> star::ManagerController::RenderResource::VertInfo::createTransferRequest(star::StarDevice &device){
    return std::make_unique<TransferRequest::VertInfo>(this->vertices);
}
