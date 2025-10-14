#include "ManagerController_RenderResource_InstanceNormalInfo.hpp"

#include "TransferRequest_InstanceNormalInfo.hpp"

std::unique_ptr<star::TransferRequest::Buffer> star::ManagerController::RenderResource::InstanceNormalInfo::createTransferRequest(star::core::device::StarDevice &device, const uint8_t &frameInFlightIndex) {
	return std::make_unique<star::TransferRequest::InstanceNormalInfo>(
		this->objectInstances, 
		device.getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex(),
		device.getPhysicalDevice().getProperties().limits.minUniformBufferOffsetAlignment
	);
}
