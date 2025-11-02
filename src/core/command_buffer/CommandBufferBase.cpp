#include "core/command_buffer/CommandBufferBase.hpp"

namespace star::core::command_buffer
{
void CommandBufferBase::prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight)
{
    m_registeredCommandBuffer = registerCommandBuffer(context, numFramesInFlight);

    assert(m_registeredCommandBuffer.isInitialized() && "Command buffer must be initialized");
}

void CommandBufferBase::cleanupRender(core::device::DeviceContext &context)
{
    // context.getManagerCommandBuffer().deleteRecord(m_registeredCommandBuffer); 
}
} // namespace star::core::command_buffer