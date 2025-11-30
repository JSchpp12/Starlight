#include "service/detail/screen_capture/BlitCmdPolicy.hpp"

namespace star::service::detail::screen_capture
{
Handle BlitCmdPolicy::registerWithManager(core::device::StarDevice &device, core::device::manager::ManagerCommandBuffer &manCmdBuf)
{
    return Handle();
}
} // namespace star::service::detail::screen_capture