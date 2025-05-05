#include "ManagerController_RenderResource_InstanceNormalInfo.hpp"

#include "TransferRequest_InstanceNormalInfo.hpp"

std::vector<std::unique_ptr<star::TransferRequest::Memory<star::StarBuffer::BufferCreationArgs>>> star::ManagerController::RenderResource::InstanceNormalInfo::createTransferRequests(const vk::PhysicalDevice& physicalDevice) {
	std::vector<std::unique_ptr<star::TransferRequest::Memory<star::StarBuffer::BufferCreationArgs>>> result;

	result.emplace_back(std::make_unique<star::TransferRequest::InstanceNormalInfo>(this->objectInstances));

	return result;
}
