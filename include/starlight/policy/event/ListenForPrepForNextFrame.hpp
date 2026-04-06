#pragma once

#include "starlight/event/StartOfNextFrame.hpp"
#include "starlight/policy/event/ListenFor.hpp"

#include <concepts>

namespace star::policy::event
{
template <typename T>
concept ValidFramePrepHandler = requires(T obj) {
    { &T::onStartOfNextFrame } -> std::same_as<void (T::*)(const star::event::StartOfNextFrame &, bool & keepAlive)>;
} || requires(T obj) {
    {
        &T::onStartOfNextFrame
    } -> std::same_as<void (T::*)(const star::event::StartOfNextFrame &, bool & keepAlive) const>;
};

template <typename T>
    requires ValidFramePrepHandler<T>
using ListenForStartOfNextFrame = ListenFor<T, star::event::StartOfNextFrame,
                                            star::event::StartOfNextFrame::GetUniqueTypeName, &T::onStartOfNextFrame>;
} // namespace star::policy::event