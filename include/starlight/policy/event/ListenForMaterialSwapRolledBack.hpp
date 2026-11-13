#pragma once

#include "starlight/event/MaterialSwapRolledBack.hpp"
#include "starlight/policy/event/ListenFor.hpp"

#include <concepts>

namespace star::policy::event
{
template <typename T>
concept ValidMaterialSwapRolledBackHandler = requires(T obj) {
    {
        &T::onMaterialSwapRolledBack
    } -> std::same_as<void (T::*)(const star::event::MaterialSwapRolledBack &, bool & keepAlive)>;
} || requires(T obj) {
    {
        &T::onMaterialSwapRolledBack
    } -> std::same_as<void (T::*)(const star::event::MaterialSwapRolledBack &, bool & keepAlive) const>;
};

template <typename T>
    requires ValidMaterialSwapRolledBackHandler<T>
using ListenForMaterialSwapRolledBack =
    ListenFor<T, star::event::MaterialSwapRolledBack, star::event::MaterialSwapRolledBack::GetUniqueTypeName,
              &T::onMaterialSwapRolledBack>;
} // namespace star::policy::event
