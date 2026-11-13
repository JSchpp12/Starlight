#pragma once

#include "starlight/event/MaterialSwapCommitted.hpp"
#include "starlight/policy/event/ListenFor.hpp"

#include <concepts>

namespace star::policy::event
{
template <typename T>
concept ValidMaterialSwapCommittedHandler = requires(T obj) {
    {
        &T::onMaterialSwapCommitted
    } -> std::same_as<void (T::*)(const star::event::MaterialSwapCommitted &, bool & keepAlive)>;
} || requires(T obj) {
    {
        &T::onMaterialSwapCommitted
    } -> std::same_as<void (T::*)(const star::event::MaterialSwapCommitted &, bool & keepAlive) const>;
};

template <typename T>
    requires ValidMaterialSwapCommittedHandler<T>
using ListenForMaterialSwapCommitted =
    ListenFor<T, star::event::MaterialSwapCommitted, star::event::MaterialSwapCommitted::GetUniqueTypeName,
              &T::onMaterialSwapCommitted>;
} // namespace star::policy::event
