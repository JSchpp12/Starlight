#include "service/detail/screen_capture/BlitCmdPolicy.hpp"

namespace star::service::detail::screen_capture
{
Handle BlitCmdPolicy::registerWithManager(core::device::StarDevice &device, core::device::manager::ManagerCommandBuffer &manCmdBuf)
{
    return Handle();
}

void BlitCmdPolicy::init(core::device::StarDevice &device)
{
    (void)device; 
}
} // namespace star::service::detail::screen_capture