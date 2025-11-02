#pragma once

#include "Handle.hpp"
#include "core/device/DeviceContext.hpp"

#include <vector>

namespace star::core::command_buffer
{
class CommandBufferBase
{
  public:
    virtual ~CommandBufferBase() = default;

    virtual void prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight);
    virtual void cleanupRender(core::device::DeviceContext &context);
    
  protected:
    virtual Handle registerCommandBuffer(core::device::DeviceContext &context,
                                                       const uint8_t &numFramesInFlight) = 0;

  private:
    Handle m_registeredCommandBuffer;
};
} // namespace star::core::command_buffer