#pragma once

#include "starlight/command/frames/GetFrameTracker.hpp"
#include "starlight/policy/command/ListenFor.hpp"

namespace star::policy::command
{
template <typename T>
using ListenForGetFrameTracker =
    ListenFor<T, frames::GetFrameTracker, frames::get_frame_tracker::GetUniqueTypeName, &T::onGetFrameTracker>;

}