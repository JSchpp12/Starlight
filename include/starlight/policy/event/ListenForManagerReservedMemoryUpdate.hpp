#pragma once

#include "starlight/event/ManagerReservedMemoryUpdate.hpp"
#include "starlight/policy/event/ListenFor.hpp"

#include <concepts>

namespace star::policy::event
{
template <typename T>
concept ValidManagerReservedMemoryUpdateHandler = requires(T obj) {
    {
        &T::onManagerReservedMemoryUpdate
    } -> std::same_as<void (T::*)(const star::event::ManagerReservedMemoryUpdate &, bool & keepAlive)>;
} || requires(T obj) {
    {
        &T::onManagerReservedMemoryUpdate
    } -> std::same_as<void (T::*)(const star::event::ManagerReservedMemoryUpdate &, bool & keepAlive) const>;
};

template <typename T>
    requires ValidManagerReservedMemoryUpdateHandler<T>
using ListenForManagerReservedMemoryUpdate =
    ListenFor<T, star::event::ManagerReservedMemoryUpdate, star::event::ManagerReservedMemoryUpdate::GetUniqueTypeName,
              &T::onManagerReservedMemoryUpdate>;
} // namespace star::policy::event
