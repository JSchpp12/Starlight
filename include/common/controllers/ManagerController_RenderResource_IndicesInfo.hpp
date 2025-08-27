#pragma once

#include "ManagerController_RenderResource_Buffer.hpp"
#include "TransferRequest_Buffer.hpp"

namespace star::ManagerController::RenderResource
{
class IndicesInfo : public Buffer
{
  public:
    IndicesInfo(const std::vector<uint32_t> &indices) : indices(indices)
    {
    }

    virtual ~IndicesInfo()
    {
    }

    std::unique_ptr<TransferRequest::Buffer> createTransferRequest(core::device::StarDevice &device) override;

  protected:
    const std::vector<uint32_t> indices;
};
} // namespace star::ManagerController::RenderResource