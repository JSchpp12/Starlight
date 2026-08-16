#pragma once

#include "ManagerController_Controller.hpp"
#include "StarBuffers/Buffer.hpp"
#include "TransferRequest_Buffer.hpp"
#include "managers/ManagerRenderResource.hpp"

#include <star_common/EventBus.hpp>

namespace star::ManagerController::RenderResource
{
class Buffer : public star::ManagerController::Controller<TransferRequest::Buffer, StarBuffers::Buffer>
{
  public:
    Buffer() = default;
    virtual ~Buffer() = default;

    /// Pipeline stage(s) the GPU must wait at before reading this buffer's
    /// latest contents after a per-frame transfer. Default covers the common
    /// vertex+fragment UBO case; light resources override to fragment-only.
    virtual vk::PipelineStageFlags waitStage() const
    {
        return vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader;
    }

  protected:
    virtual std::unique_ptr<TransferRequest::Buffer> createTransferRequest(
        core::device::DeviceContext &device, uint8_t frameInFlightIndex) override = 0;
    virtual bool doesFrameInFlightDataNeedUpdated(uint8_t frameInFlightIndex) const override = 0;
};
} // namespace star::ManagerController::RenderResource