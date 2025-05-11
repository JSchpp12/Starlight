#pragma once

#include "ManagerController_RenderResource_Buffer.hpp"
#include "Light.hpp"

#include <vector>

namespace star::ManagerController::RenderResource{
	class LightInfo : public ManagerController::RenderResource::Buffer {
        public:
        LightInfo(const uint16_t& frameInFlightIndexToUpdateOn, const std::vector<std::unique_ptr<Light>>& lights) 
        : lights(lights), 
        ManagerController::RenderResource::Buffer(static_cast<uint8_t>(frameInFlightIndexToUpdateOn)) 
        { 
        }
    
        protected:
        const std::vector<std::unique_ptr<Light>>& lights;
        uint16_t lastWriteNumLights = 0; 
    
        std::unique_ptr<TransferRequest::Buffer> createTransferRequest(StarDevice &device) override;
    
        bool isValid(const uint8_t& currentFrameInFlightIndex) const override;
    };
}