#pragma once

#include <starlight/common/Handle.hpp>
#include <starlight/common/HandleTypeRegistry.hpp>
#include <starlight/common/IEvent.hpp>

#include <utility>

namespace star::core::device::system::event
{
constexpr std::string ShaderCompiledTypeName()
{
    return "star::event::shader_compiled";
}
struct ShaderCompiled : public star::common::IEvent
{
    ShaderCompiled(Handle shaderHandle)
        : star::common::IEvent(common::HandleTypeRegistry::instance().registerType(ShaderCompiledTypeName())),
          shaderHandle(std::move(shaderHandle))
    {
    }
    Handle shaderHandle;
};
} // namespace star::core::device::system::event