#include "ManagerController_RenderResource_GlobalInfo.hpp"

#include "TransferRequest_GlobalInfo.hpp"

std::unique_ptr<star::TransferRequest::Buffer> star::ManagerController::RenderResource::GlobalInfo::createTransferRequest(star::StarDevice &device) {
    this->lastNumLights = this->numLights; 

    return std::make_unique<TransferRequest::GlobalInfo>(
        this->camera, 
        this->numLights, 
        device.getQueueFamily(star::Queue_Type::Tgraphics).getQueueFamilyIndex(), 
        device.getPhysicalDevice().getProperties().limits.minUniformBufferOffsetAlignment);
}