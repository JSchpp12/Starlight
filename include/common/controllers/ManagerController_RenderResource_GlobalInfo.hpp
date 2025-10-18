#pragma once

#include "ManagerController_RenderResource_Buffer.hpp"
#include "StarCamera.hpp"

namespace star::ManagerController::RenderResource
{
class GlobalInfo : public star::ManagerController::RenderResource::Buffer
{
  public:
    GlobalInfo(const uint8_t &numFramesInFlight, const std::shared_ptr<StarCamera> camera) : camera(camera)
    {
    }
    
    virtual ~GlobalInfo() = default;

    virtual bool needsUpdated(const uint8_t &frameInFlightIndex) const override
    {
        return true;
    }

  protected:
    const std::shared_ptr<StarCamera> camera;

    std::unique_ptr<TransferRequest::Buffer> createTransferRequest(core::device::StarDevice &device,
                                                                   const uint8_t &frameInFlightIndex) override;
};
} // namespace star::ManagerController::RenderResource