#pragma once

#include "Handle.hpp"
#include "device/system/Event.hpp"

#include <utility>

namespace star::core::device::system
{
struct ShaderCompiledEvent : public Event
{
    ShaderCompiledEvent(Handle shaderHandle) : shaderHandle(std::move(shaderHandle))
    {
    }
    Handle shaderHandle;
};
} // namespace star::core::device::system