#include "ManagerController_RenderResource_InstanceModelInfo.hpp"

#include "TransferRequest_InstanceModelInfo.hpp"

std::unique_ptr<star::TransferRequest::Buffer> star::ManagerController::RenderResource::InstanceModelInfo::createTransferRequest(star::core::device::StarDevice &device, const uint8_t &frameInFlightIndex){
    return std::make_unique<TransferRequest::InstanceModelInfo>(
        this->objectInstances,
        device.getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex(),
        device.getPhysicalDevice().getProperties().limits.minUniformBufferOffsetAlignment
    );
}