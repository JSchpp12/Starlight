#include "core/command_buffer/ScreenCapture.hpp"

namespace star::core::command_buffer
{
void ScreenCapture::prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight)
{
    
}

star::Handle ScreenCapture::registerCommandBuffer(core::device::DeviceContext &context,
                                                  const uint8_t &numFramesInFlight)
{
    return Handle();
}
} // namespace star::core::command_buffer