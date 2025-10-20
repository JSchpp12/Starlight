#pragma once

#include "ManagerController_RenderResource_Buffer.hpp"
#include "StarObjectInstance.hpp"
#include "TransferRequest_Buffer.hpp"

namespace star::ManagerController::RenderResource
{
class InstanceNormalInfo : public ManagerController::RenderResource::Buffer
{
  public:
    InstanceNormalInfo() = default;
    InstanceNormalInfo(std::shared_ptr<std::vector<StarObjectInstance>> instances) : m_instances(std::move(instances))
    {
    }
    virtual ~InstanceNormalInfo() = default;

  protected:
    std::unique_ptr<TransferRequest::Buffer> createTransferRequest(core::device::StarDevice &device,
                                                                   const uint8_t &frameInFlightIndex) override;
    bool doesFrameInFlightDataNeedUpdated(const uint8_t &frameinFlightIndex) const override
    {
        return true;
    }

  private:
    std::shared_ptr<std::vector<StarObjectInstance>> m_instances = std::shared_ptr<std::vector<StarObjectInstance>>();
};
} // namespace star::ManagerController::RenderResource