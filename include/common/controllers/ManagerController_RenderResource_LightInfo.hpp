#pragma once

#include "Light.hpp"
#include "ManagerController_RenderResource_Buffer.hpp"

#include <vector>

namespace star::ManagerController::RenderResource
{
class LightInfo : public ManagerController::RenderResource::Buffer
{
  public:
    LightInfo(const uint8_t &numFramesInFlight, const std::vector<std::shared_ptr<Light>> &lights)
        : lights(lights), lastWriteNumLights(std::vector<uint32_t>(numFramesInFlight))
    {
    }
    virtual ~LightInfo() = default;

  protected:
    const std::vector<std::shared_ptr<Light>> &lights;
    std::vector<uint32_t> lastWriteNumLights;

    std::unique_ptr<TransferRequest::Buffer> createTransferRequest(core::device::StarDevice &device,
                                                                   const uint8_t &frameInFlightIndex) override;

    bool doesFrameInFlightDataNeedUpdated(const uint8_t &currentFrameInFlightIndex) const override;
};
} // namespace star::ManagerController::RenderResource