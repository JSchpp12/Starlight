#pragma once

#include "starlight/command/shader/CreateShader.hpp"
#include "starlight/policy/command/ListenFor.hpp"

#include <concepts>

namespace star::policy::command
{
template <typename T>
concept ValidCreateShaderHandler = requires(T obj) {
    { &T::onCreateShader } -> std::same_as<void (T::*)(star::command::shader::CreateShader &)>;
};

template <typename T>
    requires ValidCreateShaderHandler<T>
using ListenForCreateShader = ListenFor<T, star::command::shader::CreateShader,
                                        star::command::shader::create_shader::GetUniqueTypeName, &T::onCreateShader>;
} // namespace star::policy::command
