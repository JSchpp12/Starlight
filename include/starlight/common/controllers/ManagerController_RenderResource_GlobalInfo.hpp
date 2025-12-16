#pragma once

#include "ManagerController_RenderResource_Buffer.hpp"
#include "StarCamera.hpp"

namespace star::ManagerController::RenderResource
{
class GlobalInfo : public star::ManagerController::RenderResource::Buffer
{
  public:
    GlobalInfo(std::shared_ptr<StarCamera> camera);

    void prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight) override;

    virtual ~GlobalInfo() = default;

  protected:
    std::shared_ptr<StarCamera> camera = nullptr;

    std::unique_ptr<TransferRequest::Buffer> createTransferRequest(core::device::StarDevice &device,
                                                                   const uint8_t &frameInFlightIndex) override;
    bool doesFrameInFlightDataNeedUpdated(const uint8_t &frameInFlightIndex) const override;
};
} // namespace star::ManagerController::RenderResource