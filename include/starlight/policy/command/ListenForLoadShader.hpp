#pragma once

#include "starlight/command/shader/LoadShader.hpp"
#include "starlight/policy/command/ListenFor.hpp"

#include <concepts>

namespace star::policy::command
{
template <typename T>
concept ValidLoadShaderHandler = requires(T obj) {
    { &T::onLoadShader } -> std::same_as<void (T::*)(star::command::shader::LoadShader &)>;
};

template <typename T>
    requires ValidLoadShaderHandler<T>
using ListenForLoadShader =
    ListenFor<T, star::command::shader::LoadShader, star::command::shader::load_shader::GetUniqueTypeName,
              &T::onLoadShader>;
} // namespace star::policy::command