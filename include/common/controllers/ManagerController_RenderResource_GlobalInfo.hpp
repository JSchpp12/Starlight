#pragma once

#include "ManagerController_RenderResource_Buffer.hpp"
#include "TransferRequest_GlobalInfo.hpp"

namespace star::ManagerController::RenderResource{
    class GlobalInfo : public star::ManagerController::RenderResource::Buffer{
        public: 

        GlobalInfo(const uint8_t& frameInFlightIndexToUpdateOn, StarCamera& camera, const int& numLights) 
        : Buffer(frameInFlightIndexToUpdateOn), camera(camera), numLights(numLights) {} 

        std::vector<std::unique_ptr<TransferRequest::Memory<StarBuffer::BufferCreationArgs>>> createTransferRequests(const vk::PhysicalDevice& physicalDevice) override;

        protected:
        const StarCamera& camera; 
        const int& numLights; 
        int lastNumLights = 0; 
    };
}