#pragma once

#include "ManagerController_Controller.hpp"
#include "StarBuffers/Buffer.hpp"
#include "TransferRequest_Buffer.hpp"
#include "managers/ManagerRenderResource.hpp"

namespace star::ManagerController::RenderResource
{
class Buffer : public star::ManagerController::Controller<TransferRequest::Buffer>
{
  public:
    Buffer() = default; 

    Buffer(bool shouldUpdateAfterCreation) : Controller<TransferRequest::Buffer>(std::move(shouldUpdateAfterCreation)){}

    Buffer(bool shouldUpdateAfterCreation, const uint8_t &numFramesInFlight) : Controller<TransferRequest::Buffer>(std::move(shouldUpdateAfterCreation), numFramesInFlight){}

    virtual ~Buffer() = default; 

  protected:
      virtual std::unique_ptr<TransferRequest::Buffer> createTransferRequest(core::device::StarDevice &device, const uint8_t &frameInFlightIndex) override = 0;
};
} // namespace star::ManagerController::RenderResource