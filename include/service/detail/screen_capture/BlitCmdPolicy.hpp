#pragma once

#include "Common.hpp"
#include "core/device/managers/ManagerCommandBuffer.hpp"

#include <starlight/common/Handle.hpp>

namespace star::service::detail::screen_capture
{
class BlitCmdPolicy
{
  public:
    explicit BlitCmdPolicy(common::InUseResourceInformation *inUseInfo) : m_inUseInfo(inUseInfo)
    {
    }
    Handle registerWithManager(core::device::StarDevice &device, core::device::manager::ManagerCommandBuffer &manCmdBuf);

  private:
    common::InUseResourceInformation *m_inUseInfo = nullptr;
};
} // namespace star::service::detail::screen_capture