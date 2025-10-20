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

  protected:
    const std::shared_ptr<StarCamera> camera;

    std::unique_ptr<TransferRequest::Buffer> createTransferRequest(core::device::StarDevice &device,
                                                                   const uint8_t &frameInFlightIndex) override;
    bool doesFrameInFlightDataNeedUpdated(const uint8_t &frameInFlightIndex) const override
    {
        return true;
    }
};
} // namespace star::ManagerController::RenderResource