#pragma once

#include "ManagerController_RenderResource_Buffer.hpp"
#include "StarObjectInstance.hpp"

namespace star::ManagerController::RenderResource{
    class InstanceNormalInfo : public ManagerController::RenderResource::Buffer{
        public:
		InstanceNormalInfo(const std::vector<std::unique_ptr<StarObjectInstance>>& objectInstances, const int& frameInFlightToUpdateOn)
			: ManagerController::RenderResource::Buffer(static_cast<uint8_t>(frameInFlightToUpdateOn)),
			objectInstances(objectInstances) {}

        protected:
            std::unique_ptr<TransferRequest::Memory<StarBuffer::BufferCreationArgs>> createTransferRequest(const vk::PhysicalDevice& physicalDevice) override;

        private:
            const std::vector<std::unique_ptr<StarObjectInstance>>& objectInstances;

    };
}