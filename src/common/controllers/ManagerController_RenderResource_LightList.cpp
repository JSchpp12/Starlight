#include "ManagerController_RenderResource_LightList.hpp"

#include <starlight/common/helper/CastHelpers.hpp>
#include "TransferRequest_LightList.hpp"

std::unique_ptr<star::TransferRequest::Buffer> star::ManagerController::RenderResource::LightList::
    createTransferRequest(core::device::StarDevice &device, const uint8_t &frameInFlightIndex)
{
    storeLightCount(frameInFlightIndex);

    return std::make_unique<TransferRequest::LightList>(
        *m_lights, device.getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex());
}

bool star::ManagerController::RenderResource::LightList::doesFrameInFlightDataNeedUpdated(const uint8_t &frameInFlightIndex) const
{

    return true; 
    assert(frameInFlightIndex < m_lastWriteNumLights.size());

    return m_lastWriteNumLights[frameInFlightIndex] != m_lights->size(); 
}

void star::ManagerController::RenderResource::LightList::storeLightCount(const uint8_t &frameInFlightIndex)
{
    assert(frameInFlightIndex < m_lastWriteNumLights.size());
    
    uint16_t numLights = 0;
    common::helper::SafeCast<size_t, uint16_t>(m_lights->size(), numLights);

    m_lastWriteNumLights[frameInFlightIndex] = std::move(numLights);
}