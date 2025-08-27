#pragma once

#include "Light.hpp"
#include "ManagerController_RenderResource_Buffer.hpp"

#include <vector>

namespace star::ManagerController::RenderResource
{
class LightInfo : public ManagerController::RenderResource::Buffer
{
  public:
    LightInfo(const uint16_t &frameInFlightIndexToUpdateOn, const std::vector<std::unique_ptr<Light>> &lights)
        : ManagerController::RenderResource::Buffer(static_cast<uint8_t>(frameInFlightIndexToUpdateOn)), lights(lights)
    {
    }

  protected:
    const std::vector<std::unique_ptr<Light>> &lights;
    uint16_t lastWriteNumLights = 0;

    std::unique_ptr<TransferRequest::Buffer> createTransferRequest(core::device::StarDevice &device) override;

    bool isValid(const uint8_t &currentFrameInFlightIndex) const override;
};
} // namespace star::ManagerController::RenderResource