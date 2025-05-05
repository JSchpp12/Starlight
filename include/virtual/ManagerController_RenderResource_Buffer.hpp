#pragma once

#include "StarBuffer.hpp"
#include "TransferRequest_Buffer.hpp"
#include "ManagerController_Controller.hpp"

namespace star::ManagerController::RenderResource{
    class Buffer : public star::ManagerController::Controller<TransferRequest::Buffer> {
    public:
    Buffer() = default;

    Buffer(const uint8_t& frameInFlightIndexToUpdateOn) 
    : star::ManagerController::Controller<TransferRequest::Buffer>(frameInFlightIndexToUpdateOn){}; 

    virtual std::unique_ptr<TransferRequest::Buffer> createTransferRequest(const vk::PhysicalDevice& physicalDevice) override = 0;

    protected:

    };
}