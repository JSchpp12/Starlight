#pragma once

#include "Light.hpp"
#include "ManagerController_RenderResource_Buffer.hpp"


namespace star::ManagerController::RenderResource
{
class LightList : public ManagerController::RenderResource::Buffer
{
  public:
    LightList(const uint16_t &frameInFlightIndexToUpdateOn, const std::vector<std::shared_ptr<Light>> &lights)
        : ManagerController::RenderResource::Buffer(static_cast<uint8_t>(frameInFlightIndexToUpdateOn)), m_lights(lights)
    {
    }

  private:
    const std::vector<std::shared_ptr<Light>> &m_lights;
    uint16_t m_lastWriteNumLights = 0;

    std::unique_ptr<TransferRequest::Buffer> createTransferRequest(core::device::StarDevice &device) override;

    bool isValid(const uint8_t &currentFrameInFlightIndex) const override;
};
} // namespace star::ManagerController::RenderResource