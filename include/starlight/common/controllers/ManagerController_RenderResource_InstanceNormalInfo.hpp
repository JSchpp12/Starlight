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
    InstanceNormalInfo(std::shared_ptr<std::vector<StarObjectInstance>> instances);
    virtual ~InstanceNormalInfo() = default;

    void setForUpdate();

    void prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight) override;

  protected:
    std::unique_ptr<TransferRequest::Buffer> createTransferRequest(core::device::DeviceContext &context,
                                                                   const uint8_t &frameInFlightIndex) override;
    bool doesFrameInFlightDataNeedUpdated(const uint8_t &frameinFlightIndex) const override;

  private:
    std::shared_ptr<std::vector<StarObjectInstance>> m_instances = nullptr;
    std::vector<bool> m_needsUpdatedThisFrame;
};
} // namespace star::ManagerController::RenderResource