#pragma once

#include "Light.hpp"
#include "ManagerController_RenderResource_Buffer.hpp"

namespace star::ManagerController::RenderResource
{
class LightList : public ManagerController::RenderResource::Buffer
{
  public:
    LightList(const uint8_t &numFramesInFlight, std::shared_ptr<std::vector<Light>> lights)
        : m_lights(std::move(lights)), m_lastWriteNumLights(std::vector<uint16_t>(numFramesInFlight))
    {
    }

    virtual ~LightList() = default;

  protected:
    bool doesFrameInFlightDataNeedUpdated(const uint8_t &frameInFlightIndex) const override;

  private:
    const std::shared_ptr<std::vector<Light>> m_lights;
    std::vector<uint16_t> m_lastWriteNumLights;

    std::unique_ptr<TransferRequest::Buffer> createTransferRequest(core::device::StarDevice &device,
                                                                   const uint8_t &frameInFlightIndex) override;

    void storeLightCount(const uint8_t &frameInFlightIndex);
};
} // namespace star::ManagerController::RenderResource