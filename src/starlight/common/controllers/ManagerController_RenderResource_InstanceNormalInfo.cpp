#include "ManagerController_RenderResource_InstanceNormalInfo.hpp"

#include "TransferRequest_InstanceNormalInfo.hpp"

#include <cassert>

star::ManagerController::RenderResource::InstanceNormalInfo::InstanceNormalInfo(
    std::shared_ptr<std::vector<StarObjectInstance>> instances)
    : m_instances(std::move(instances))
{
    assert(m_instances && "Instances must be valid");
}

void star::ManagerController::RenderResource::InstanceNormalInfo::setForUpdate()
{
    for (size_t i = 0; i < m_needsUpdatedThisFrame.size(); i++)
    {
        m_needsUpdatedThisFrame[i] = true;
    }
}

void star::ManagerController::RenderResource::InstanceNormalInfo::prepRender(core::device::DeviceContext &context,
                                                                             const uint8_t &numFramesInFlight)
{
    m_needsUpdatedThisFrame.resize(numFramesInFlight, true);

    Buffer::prepRender(context, numFramesInFlight);
}

std::unique_ptr<star::TransferRequest::Buffer> star::ManagerController::RenderResource::InstanceNormalInfo::
    createTransferRequest(star::core::device::StarDevice &device, const uint8_t &frameInFlightIndex)
{
    assert(m_instances && "Instances must be provided before use");

    m_needsUpdatedThisFrame[frameInFlightIndex] = false;

    return std::make_unique<star::TransferRequest::InstanceNormalInfo>(
        *m_instances, device.getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex(),
        device.getPhysicalDevice().getProperties().limits.minUniformBufferOffsetAlignment);
}

bool star::ManagerController::RenderResource::InstanceNormalInfo::doesFrameInFlightDataNeedUpdated(
    const uint8_t &frameInFlightIndex) const
{
    return m_needsUpdatedThisFrame[frameInFlightIndex];
}
