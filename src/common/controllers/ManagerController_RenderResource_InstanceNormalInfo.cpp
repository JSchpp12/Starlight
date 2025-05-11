#include "ManagerController_RenderResource_InstanceNormalInfo.hpp"

#include "TransferRequest_InstanceNormalInfo.hpp"

std::unique_ptr<star::TransferRequest::Buffer> star::ManagerController::RenderResource::InstanceNormalInfo::createTransferRequest(star::StarDevice &device) {
	return std::make_unique<star::TransferRequest::InstanceNormalInfo>(this->objectInstances);
}
