#pragma once 

#include "ManagerController_RenderResource_Buffer.hpp"
#include "StarObjectInstance.hpp"

namespace star::ManagerController::RenderResource{
    class InstanceModelInfo : public Buffer{
        public:
        InstanceModelInfo(const std::vector<std::unique_ptr<star::StarObjectInstance>>& objectInstances, const int& frameInFlightToUpdateOn)
            : ManagerController::RenderResource::Buffer(static_cast<uint8_t>(frameInFlightToUpdateOn)), objectInstances(objectInstances)
        {
        }
    
        std::unique_ptr<TransferRequest::Memory<StarBuffer::BufferCreationArgs>> createTransferRequest(const vk::PhysicalDevice& physicalDevice) override;
        private:
        const std::vector<std::unique_ptr<StarObjectInstance>>& objectInstances; 
    
    };
}