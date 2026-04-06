#pragma once

#include "starlight/event/FrameComplete.hpp"
#include "starlight/policy/event/ListenFor.hpp"

#include <concepts>

namespace star::policy::event
{
template <typename T>
concept ValidFrameCompleteHandler = requires(T obj) {
    { &T::onFrameComplete } -> std::same_as<void (T::*)(const star::event::FrameComplete &, bool & keepAlive)>;
} || requires(T obj) {
    { &T::onFrameComplete } -> std::same_as<void (T::*)(const star::event::FrameComplete &, bool & keepAlive) const>;
};

template <typename T>
    requires ValidFrameCompleteHandler<T>
using ListenForFrameComplete =
    ListenFor<T, star::event::FrameComplete, star::event::FrameComplete::GetUniqueTypeName, &T::onFrameComplete>;
} // namespace star::policy::event