#pragma once

#include "StarTexture.hpp"
#include "TransferRequest_Memory.hpp"
#include "ManagerController_Controller.hpp"

namespace star::ManagerController::RenderResource{
class Texture : public star::ManagerController::Controller<TransferRequest::Memory<StarTexture::TextureCreateSettings>>{
        public:
        Texture() = default; 

        Texture(const uint8_t& frameInFlightIndexToUpdateOn)
            : star::ManagerController::Controller<TransferRequest::Memory<StarTexture::TextureCreateSettings>>(frameInFlightIndexToUpdateOn){}

        virtual std::vector<std::unique_ptr<TransferRequest::Memory<StarTexture::TextureCreateSettings>>> createTransferRequests(const vk::PhysicalDevice& physicalDevice) override = 0;

        protected:

    };
}