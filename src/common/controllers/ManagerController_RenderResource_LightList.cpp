#include "ManagerController_RenderResource_LightList.hpp"

#include "CastHelpers.hpp"
#include "TransferRequest_LightList.hpp"

std::unique_ptr<star::TransferRequest::Buffer> star::ManagerController::RenderResource::LightList::
    createTransferRequest(core::device::StarDevice &device, const uint8_t &frameInFlightIndex)
{
    storeLightCount(frameInFlightIndex);

    return std::make_unique<TransferRequest::LightList>(
        this->m_lights, device.getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex());
}

bool star::ManagerController::RenderResource::LightList::needsUpdated(const uint8_t &frameInFlightIndex) const
{
    assert(frameInFlightIndex < m_lastWriteNumLights.size());

    return m_lastWriteNumLights[frameInFlightIndex] != m_lights.size(); 
}

void star::ManagerController::RenderResource::LightList::storeLightCount(const uint8_t &frameInFlightIndex)
{
    assert(frameInFlightIndex < m_lastWriteNumLights.size());
    
    uint16_t numLights = 0;
    CastHelpers::SafeCast<size_t, uint16_t>(m_lights.size(), numLights);

    m_lastWriteNumLights[frameInFlightIndex] = std::move(numLights);
}