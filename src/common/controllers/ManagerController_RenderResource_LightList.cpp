#include "ManagerController_RenderResource_LightList.hpp"

#include "TransferRequest_LightList.hpp"

std::unique_ptr<star::TransferRequest::Buffer> star::ManagerController::RenderResource::LightList::
    createTransferRequest(core::device::StarDevice &device)
{
    m_lastWriteNumLights = this->m_lights.size();

    return std::make_unique<TransferRequest::LightList>(
        this->m_lights, device.getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex());
}

bool star::ManagerController::RenderResource::LightList::isValid(const uint8_t &currentFrameInFlightIndex) const
{
    if (!star::ManagerController::RenderResource::Buffer::isValid(currentFrameInFlightIndex) &&
        m_lastWriteNumLights != m_lights.size())
    {
        return false;
    }

    return true;
}
