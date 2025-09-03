#pragma once

#include "ManagerController_RenderResource_Buffer.hpp"
#include "StarCamera.hpp"

namespace star::ManagerController::RenderResource{
    class GlobalInfo : public star::ManagerController::RenderResource::Buffer{
        public: 

        GlobalInfo(const uint8_t& frameInFlightIndexToUpdateOn, const std::shared_ptr<StarCamera> camera) 
        : Buffer(frameInFlightIndexToUpdateOn), camera(camera){} 

        std::unique_ptr<TransferRequest::Buffer> createTransferRequest(core::device::StarDevice &device) override;

        protected:
        const std::shared_ptr<StarCamera> camera; 
    };
}