#include "ManagerController_RenderResource_InstanceModelInfo.hpp"

#include "TransferRequest_InstanceModelInfo.hpp"

#include <cassert>

std::unique_ptr<star::TransferRequest::Buffer> star::ManagerController::RenderResource::InstanceModelInfo::createTransferRequest(star::core::device::StarDevice &device, const uint8_t &frameInFlightIndex){
    assert(m_instances && "Instances must be provided before use"); 

    return std::make_unique<TransferRequest::InstanceModelInfo>(
        *m_instances,
        device.getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex(),
        device.getPhysicalDevice().getProperties().limits.minUniformBufferOffsetAlignment
    );
}