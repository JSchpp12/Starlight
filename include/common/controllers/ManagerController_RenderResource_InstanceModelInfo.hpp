#pragma once

#include "ManagerController_RenderResource_Buffer.hpp"
#include "StarObjectInstance.hpp"

namespace star::ManagerController::RenderResource
{
class InstanceModelInfo : public Buffer
{
  public:
    InstanceModelInfo() = default;
    InstanceModelInfo(std::shared_ptr<std::vector<StarObjectInstance>> instances) : m_instances(std::move(instances))
    {
    }
    virtual ~InstanceModelInfo() = default;

  protected:
    std::unique_ptr<TransferRequest::Buffer> createTransferRequest(core::device::StarDevice &device,
                                                                   const uint8_t &frameInFlightIndex) override;

    bool doesFrameInFlightDataNeedUpdated(const uint8_t &frameInFlightIndex) const override
    {
        return true;
    }

  private:
    std::shared_ptr<std::vector<StarObjectInstance>> m_instances = std::shared_ptr<std::vector<StarObjectInstance>>();
};
} // namespace star::ManagerController::RenderResource