#pragma once 

#include "ManagerController_RenderResource_Buffer.hpp"
#include "StarObjectInstance.hpp"

namespace star::ManagerController::RenderResource{
    class InstanceModelInfo : public Buffer{
        public:
        InstanceModelInfo(const uint8_t &numFramesInFlight, const std::vector<std::unique_ptr<star::StarObjectInstance>>& objectInstances)
            : ManagerController::RenderResource::Buffer(true, numFramesInFlight), objectInstances(objectInstances)
        {
        }
    
        std::unique_ptr<TransferRequest::Buffer> createTransferRequest(core::device::StarDevice &device, const uint8_t &frameInFlightIndex) override;
        private:
        const std::vector<std::unique_ptr<StarObjectInstance>>& objectInstances; 
    
    };
}