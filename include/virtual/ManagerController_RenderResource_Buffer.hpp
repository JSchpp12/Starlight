#pragma once

#include "StarBuffer.hpp"
#include "ManagerController_Controller.hpp"
#include "TransferRequest_Memory.hpp"

namespace star::ManagerController::RenderResource{
    class Buffer : public star::ManagerController::Controller<TransferRequest::Memory<StarBuffer::BufferCreationArgs>> {
    public:
    Buffer() = default;

    Buffer(const uint8_t& frameInFlightIndexToUpdateOn) 
    : star::ManagerController::Controller<TransferRequest::Memory<StarBuffer::BufferCreationArgs>>(frameInFlightIndexToUpdateOn){}; 

    virtual std::vector<std::unique_ptr<TransferRequest::Memory<StarBuffer::BufferCreationArgs>>> createTransferRequests(const vk::PhysicalDevice& physicalDevice) override = 0;

    protected:

    };
}