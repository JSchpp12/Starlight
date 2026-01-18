#include "ManagerController_RenderResource_GlobalInfo.hpp"

#include "TransferRequest_GlobalInfo.hpp"
#include "core/helper/queue/QueueHelpers.hpp"

star::ManagerController::RenderResource::GlobalInfo::GlobalInfo(std::shared_ptr<StarCamera> camera) : camera(camera)
{
}

void star::ManagerController::RenderResource::GlobalInfo::prepRender(core::device::DeviceContext &context,
                                                                     const uint8_t &numFramesInFlight)
{
    Buffer::prepRender(context, numFramesInFlight);
}

std::unique_ptr<star::TransferRequest::Buffer> star::ManagerController::RenderResource::GlobalInfo::
    createTransferRequest(star::core::device::DeviceContext &context, const uint8_t &frameInFlightIndex)
{
    return std::make_unique<TransferRequest::GlobalInfo>(
        *this->camera,
        core::helper::GetEngineDefaultQueue(context.getEventBus(), context.getGraphicsManagers().queueManager,
                                            star::Queue_Type::Tgraphics)
            ->getParentQueueFamilyIndex());
}

bool star::ManagerController::RenderResource::GlobalInfo::doesFrameInFlightDataNeedUpdated(
    const uint8_t &frameInFlightIndex) const
{
    return true;
}
