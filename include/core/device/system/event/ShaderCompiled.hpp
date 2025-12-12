#pragma once

#include <starlight/common/Handle.hpp>
#include <starlight/common/HandleTypeRegistry.hpp>
#include <starlight/common/IEvent.hpp>

#include <utility>

namespace star::core::device::system::event
{
constexpr std::string_view GetShaderCompiledEventTypeName = "star::core::device::event::ShaderCompiled";
struct ShaderCompiled : public star::common::IEvent
{
    ShaderCompiled(Handle shaderHandle)
        : star::common::IEvent(common::HandleTypeRegistry::instance().registerType(GetShaderCompiledEventTypeName)),
          shaderHandle(std::move(shaderHandle))
    {
    }
    Handle shaderHandle;
};
} // namespace star::core::device::system::event