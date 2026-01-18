#pragma once

#include "ManagerController_RenderResource_Buffer.hpp"
#include "StarObjectInstance.hpp"

namespace star::ManagerController::RenderResource
{
class InstanceModelInfo : public Buffer
{
  public:
    InstanceModelInfo() = default;
    InstanceModelInfo(std::shared_ptr<std::vector<StarObjectInstance>> instances);

    virtual ~InstanceModelInfo() = default;

    void prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight) override;

    void setToUpdate();

  protected:
    std::unique_ptr<TransferRequest::Buffer> createTransferRequest(core::device::DeviceContext &context,
                                                                   const uint8_t &frameInFlightIndex) override;

    bool doesFrameInFlightDataNeedUpdated(const uint8_t &frameInFlightIndex) const override;

  private:
    std::shared_ptr<std::vector<StarObjectInstance>> m_instances = std::shared_ptr<std::vector<StarObjectInstance>>();
    std::vector<bool> m_needsUpdatedThisFrame;
};
} // namespace star::ManagerController::RenderResource