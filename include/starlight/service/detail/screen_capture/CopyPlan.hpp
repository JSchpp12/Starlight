#pragma once

#include "Common.hpp"
#include "CalleeRenderDependencies.hpp"
#include "PerExtentResources.hpp"

namespace star::service::detail::screen_capture
{
struct CopyPlan
{
    CopyResource resources;
    common::RoutePath path;
    vk::Filter blitFilter;
    CalleeRenderDependencies *calleeDependencies = nullptr;
};
} // namespace star::service::detail::screen_capture