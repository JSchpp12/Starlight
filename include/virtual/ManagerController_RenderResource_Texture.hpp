#pragma once

#include "StarTexture.hpp"
#include "TransferRequest_Texture.hpp"
#include "ManagerController_Controller.hpp"

namespace star::ManagerController::RenderResource{
class Texture : public star::ManagerController::Controller<TransferRequest::Texture>{
        public:
        Texture() = default; 

        Texture(const uint8_t& frameInFlightIndexToUpdateOn)
            : star::ManagerController::Controller<TransferRequest::Texture>(frameInFlightIndexToUpdateOn){}

        virtual std::unique_ptr<TransferRequest::Texture> createTransferRequest(StarDevice& device) override = 0;

        protected:

    };
}