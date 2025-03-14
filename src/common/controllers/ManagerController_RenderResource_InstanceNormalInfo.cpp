#include "ManagerController_RenderResource_InstanceNormalInfo.hpp"

#include "TransferRequest_InstanceNormalInfo.hpp"

std::unique_ptr<star::TransferRequest::Memory<star::StarBuffer::BufferCreationArgs>> star::ManagerController::RenderResource::InstanceNormalInfo::createTransferRequest() {
	return std::make_unique<star::TransferRequest::InstanceNormalInfo>(this->objectInstances);
}
