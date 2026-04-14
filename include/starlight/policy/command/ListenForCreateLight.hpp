#pragma once
#include "starlight/command/CreateLight.hpp"
#include "starlight/policy/command/ListenFor.hpp"

#include <concepts>

namespace star::policy::command
{
template <typename T>
concept ValidCreateLightHandler = requires(T obj) {
    { &T::onCreateLight } -> std::same_as<void (T::*)(star::command::CreateLight &)>;
};

template <typename T>
    requires ValidCreateLightHandler<T>
using ListenForCreateLight =
    ListenFor<T, star::command::CreateLight, star::command::create_light::GetUniqueTypeName, &T::onCreateLight>;

} // namespace star::policy::command