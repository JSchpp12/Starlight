#pragma once

#include "starlight/event/PassCPURecordingWaitResolved.hpp"
#include "starlight/policy/event/ListenFor.hpp"

#include <concepts>

namespace star::policy::event
{
template <typename T>
concept ValidPassCPURecordingWaitResolvedHandler = requires(T obj) {
    {
        &T::onPassCPURecordingWaitResolved
    } -> std::same_as<void (T::*)(const star::event::PassCPURecordingWaitResolved &, bool & keepAlive)>;
} || requires(T obj) {
    {
        &T::onPassCPURecordingWaitResolved
    } -> std::same_as<void (T::*)(const star::event::PassCPURecordingWaitResolved &, bool & keepAlive) const>;
};

template <typename T>
    requires ValidPassCPURecordingWaitResolvedHandler<T>
using ListenForPassCPURecordingWaitResolved =
    ListenFor<T, star::event::PassCPURecordingWaitResolved,
              star::event::PassCPURecordingWaitResolved::GetUniqueTypeName, &T::onPassCPURecordingWaitResolved>;
} // namespace star::policy::event
