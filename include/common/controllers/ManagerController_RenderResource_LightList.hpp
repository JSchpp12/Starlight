#pragma once

#include "Light.hpp"
#include "ManagerController_RenderResource_Buffer.hpp"

namespace star::ManagerController::RenderResource
{
class LightList : public ManagerController::RenderResource::Buffer
{
  public:
    LightList(const uint8_t &numFramesInFlight, const std::vector<std::shared_ptr<Light>> &lights)
        : ManagerController::RenderResource::Buffer(true, numFramesInFlight), m_lights(lights),
          m_lastWriteNumLights(std::vector<uint16_t>(numFramesInFlight))
    {
    }

    virtual ~LightList() = default;

  private:
    const std::vector<std::shared_ptr<Light>> &m_lights;
    std::vector<uint16_t> m_lastWriteNumLights;

    std::unique_ptr<TransferRequest::Buffer> createTransferRequest(core::device::StarDevice &device,
                                                                   const uint8_t &frameInFlightIndex) override;

    bool isValid(const uint8_t &frameInFlightIndex) const override;

    void storeLightCount(const uint8_t &frameInFlightIndex); 
};
} // namespace star::ManagerController::RenderResource