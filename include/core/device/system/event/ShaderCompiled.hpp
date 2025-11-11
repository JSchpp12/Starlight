#pragma once

#include "Handle.hpp"

#include <starlight/common/IEvent.hpp>

#include <utility>

namespace star::core::device::system::event
{
struct ShaderCompiled : public star::common::IEvent
{
    ShaderCompiled(Handle shaderHandle) : shaderHandle(std::move(shaderHandle))
    {
    }
    Handle shaderHandle;
};
} // namespace star::core::device::system