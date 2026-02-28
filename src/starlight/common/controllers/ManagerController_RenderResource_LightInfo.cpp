#include "ManagerController_RenderResource_LightInfo.hpp"

#include "TransferRequest_LightInfo.hpp"
#include "core/helper/queue/QueueHelpers.hpp"

std::unique_ptr<star::TransferRequest::Buffer> star::ManagerController::RenderResource::LightInfo::
    createTransferRequest(star::core::device::DeviceContext &context, const uint8_t &frameInFlightIndex)
{
    uint32_t numLights;
    star::common::helper::SafeCast<size_t, uint32_t>(this->lights->size(), numLights);

    this->lastWriteNumLights[frameInFlightIndex] = numLights;

    return std::make_unique<TransferRequest::LightInfo>(
        numLights, core::helper::GetEngineDefaultQueue(
                       context.getEventBus(), context.getGraphicsManagers().queueManager, star::Queue_Type::Tgraphics)
                       ->getParentQueueFamilyIndex());
}

bool star::ManagerController::RenderResource::LightInfo::doesFrameInFlightDataNeedUpdated(
    const uint8_t &currentFrameInFlightIndex) const
{
    assert(currentFrameInFlightIndex < lastWriteNumLights.size() && "Not enough resources were created for this");

    if (lastWriteNumLights[currentFrameInFlightIndex] != lights->size())
    {
        return true;
    }

    return false;
}