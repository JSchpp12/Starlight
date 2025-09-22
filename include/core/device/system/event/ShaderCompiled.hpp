#pragma once

#include "Handle.hpp"
#include "device/system/event/EventBase.hpp"

#include <utility>

namespace star::core::device::system::event
{
struct ShaderCompiled : public EventBase
{
    ShaderCompiled(Handle shaderHandle) : shaderHandle(std::move(shaderHandle))
    {
    }
    Handle shaderHandle;
};
} // namespace star::core::device::system