#pragma once

#include "ManagerController_Controller.hpp"
#include "StarBuffers/Buffer.hpp"
#include "TransferRequest_Buffer.hpp"
#include "managers/ManagerRenderResource.hpp"

namespace star::ManagerController::RenderResource
{
class Buffer : public star::ManagerController::Controller<TransferRequest::Buffer>
{
  public:
    Buffer() = default;

    virtual ~Buffer() = default;

    virtual bool submitUpdateIfNeeded(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex,
                                      vk::Semaphore &semaphore) override
    {
        if (!needsUpdated(frameInFlightIndex))
        {
            return false;
        }

        context.getManagerRenderResource().updateRequest(context.getDeviceID(),
                                                         createTransferRequest(context.getDevice(), frameInFlightIndex),
                                                         m_resourceHandles[frameInFlightIndex], true);

        semaphore = context.getManagerRenderResource()
                        .get<StarBuffers::Buffer>(context.getDeviceID(), m_resourceHandles[frameInFlightIndex])
                        ->resourceSemaphore;

        return true;
    }

  protected:
    virtual std::unique_ptr<TransferRequest::Buffer> createTransferRequest(
        core::device::StarDevice &device, const uint8_t &frameInFlightIndex) override = 0;
};
} // namespace star::ManagerController::RenderResource