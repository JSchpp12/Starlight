#pragma once

#include "ManagerController_RenderResource_Buffer.hpp"
#include "StarCamera.hpp"

namespace star::ManagerController::RenderResource{
    class GlobalInfo : public star::ManagerController::RenderResource::Buffer{
        public: 

        GlobalInfo(const uint8_t& frameInFlightIndexToUpdateOn, StarCamera& camera, const int& numLights) 
        : Buffer(frameInFlightIndexToUpdateOn), camera(camera), numLights(numLights) {} 

        std::unique_ptr<TransferRequest::Buffer> createTransferRequest(const vk::PhysicalDevice& physicalDevice) override;

        protected:
        const StarCamera& camera; 
        const int& numLights; 
        int lastNumLights = 0; 
    };
}