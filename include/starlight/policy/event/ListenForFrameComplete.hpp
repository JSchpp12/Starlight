#pragma once

#include "starlight/event/FrameComplete.hpp"
#include "starlight/policy/event/ListenFor.hpp"

namespace star::policy::event
{
template <typename T>
using ListenForFrameComplete =
    ListenFor<T, star::event::FrameComplete, star::event::FrameComplete::GetUniqueTypeName, &T::onFrameComplete>;
}