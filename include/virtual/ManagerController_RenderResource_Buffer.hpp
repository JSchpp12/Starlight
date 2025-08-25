#pragma once

#include "ManagerController_Controller.hpp"
#include "StarBuffers/Buffer.hpp"
#include "TransferRequest_Buffer.hpp"


namespace star::ManagerController::RenderResource
{
class Buffer : public star::ManagerController::Controller<TransferRequest::Buffer>
{
  public:
    Buffer() = default;

    Buffer(const uint8_t &frameInFlightIndexToUpdateOn)
        : star::ManagerController::Controller<TransferRequest::Buffer>(frameInFlightIndexToUpdateOn) {};

    virtual ~Buffer() = default; 

    virtual std::unique_ptr<TransferRequest::Buffer> createTransferRequest(core::devices::StarDevice &device) override = 0;

  protected:
};
} // namespace star::ManagerController::RenderResource