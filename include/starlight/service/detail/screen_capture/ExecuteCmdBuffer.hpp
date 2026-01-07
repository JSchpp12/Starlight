#pragma once

#include "core/device/managers/ManagerCommandBuffer.hpp"

#include <star_common/Handle.hpp>

#include <vulkan/vulkan.hpp>

#include <concepts>

namespace star::service::detail::screen_capture
{
template <typename TPolicy>
concept TExecutePolicyLike =
    requires(TPolicy c, core::device::StarDevice &device, core::device::manager::ManagerCommandBuffer &manCmdBuf) {
        { c.registerWithManager(device, manCmdBuf) } -> std::same_as<Handle>;
        { c.init(device) } -> std::same_as<void>;
    };

template <TExecutePolicyLike TExecuteBufferPolicy> class ExecuteCmdBuffer
{
  public:
    explicit ExecuteCmdBuffer(TExecuteBufferPolicy executePolicy) : m_executePolicy(std::move(executePolicy))
    {
    }

    void init(core::device::StarDevice &device, core::device::manager::ManagerCommandBuffer &manCmdBuf)
    {
        m_commandBuffer = m_executePolicy.registerWithManager(device, manCmdBuf);
        m_executePolicy.init(device);
    }

    void trigger(core::device::manager::ManagerCommandBuffer &manCmdBuf)
    {
        manCmdBuf.submitDynamicBuffer(m_commandBuffer);
    }

    Handle &getCommandBuffer()
    {
        return m_commandBuffer;
    }

    const Handle &getCommandBuffer() const
    {
        return m_commandBuffer;
    }

  private:
    TExecuteBufferPolicy m_executePolicy;
    Handle m_commandBuffer;
};
} // namespace star::service::detail::screen_capture