#pragma once

#include "starlight/command/frames/GetFrameTracker.hpp"
#include "starlight/policy/command/ListenFor.hpp"

#include <concepts>

namespace star::policy::command
{
template <typename T>
concept ValidGetFrameTrackerHandler = requires(T obj) {
    { &T::onGetFrameTracker } -> std::same_as<void (T::*)(star::frames::GetFrameTracker &)>;
} || requires(T obj) {
    { &T::onGetFrameTracker } -> std::same_as<void (T::*)(star::frames::GetFrameTracker &) const>;
};

template <typename T>
    requires ValidGetFrameTrackerHandler<T>
using ListenForGetFrameTracker =
    ListenFor<T, frames::GetFrameTracker, frames::get_frame_tracker::GetUniqueTypeName, &T::onGetFrameTracker>;
} // namespace star::policy::command