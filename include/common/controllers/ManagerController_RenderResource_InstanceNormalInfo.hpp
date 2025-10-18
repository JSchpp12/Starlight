#pragma once

#include "ManagerController_RenderResource_Buffer.hpp"
#include "StarObjectInstance.hpp"
#include "TransferRequest_Buffer.hpp"

namespace star::ManagerController::RenderResource
{
class InstanceNormalInfo : public ManagerController::RenderResource::Buffer
{
  public:
    InstanceNormalInfo(const uint8_t &numFramesInFlight,
                       const std::vector<std::unique_ptr<StarObjectInstance>> &objectInstances)
        : objectInstances(objectInstances)
    {
    }

    virtual bool needsUpdated(const uint8_t &frameInFlightIndex) const override
    {
        return true;
    }

  protected:
    std::unique_ptr<TransferRequest::Buffer> createTransferRequest(core::device::StarDevice &device,
                                                                   const uint8_t &frameInFlightIndex) override;

  private:
    const std::vector<std::unique_ptr<StarObjectInstance>> &objectInstances;
};
} // namespace star::ManagerController::RenderResource