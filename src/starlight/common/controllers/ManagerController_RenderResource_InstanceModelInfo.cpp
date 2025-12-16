#include "ManagerController_RenderResource_InstanceModelInfo.hpp"

#include "TransferRequest_InstanceModelInfo.hpp"

#include <cassert>

star::ManagerController::RenderResource::InstanceModelInfo::InstanceModelInfo(
    std::shared_ptr<std::vector<StarObjectInstance>> instances)
    : m_instances(std::move(instances))
{
    assert(m_instances && "Instances must be provided before use");
}

void star::ManagerController::RenderResource::InstanceModelInfo::prepRender(core::device::DeviceContext &context,
                                                                            const uint8_t &numFramesInFlight)
{
    m_needsUpdatedThisFrame.resize(numFramesInFlight, true);

    Buffer::prepRender(context, numFramesInFlight);
}

void star::ManagerController::RenderResource::InstanceModelInfo::setToUpdate()
{
    for (size_t i = 0; i < m_needsUpdatedThisFrame.size(); i++)
    {
        m_needsUpdatedThisFrame[i] = true;
    }
}

std::unique_ptr<star::TransferRequest::Buffer> star::ManagerController::RenderResource::InstanceModelInfo::
    createTransferRequest(star::core::device::StarDevice &device, const uint8_t &frameInFlightIndex)
{
    m_needsUpdatedThisFrame[frameInFlightIndex] = false;

    return std::make_unique<TransferRequest::InstanceModelInfo>(
        *m_instances, device.getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex(),
        device.getPhysicalDevice().getProperties().limits.minUniformBufferOffsetAlignment);
}

bool star::ManagerController::RenderResource::InstanceModelInfo::doesFrameInFlightDataNeedUpdated(
    const uint8_t &frameInFlightIndex) const
{
    assert(frameInFlightIndex < m_needsUpdatedThisFrame.size());

    return m_needsUpdatedThisFrame[frameInFlightIndex];
}
