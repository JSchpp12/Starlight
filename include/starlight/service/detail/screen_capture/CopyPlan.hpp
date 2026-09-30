#pragma once

#include "CalleeRenderDependencies.hpp"
#include "PerExtentResources.hpp"

namespace star::service::detail::screen_capture
{
struct CopyPlan
{
    CopyResource resources;
    CalleeRenderDependencies *calleeDependencies = nullptr;
};
} // namespace star::service::detail::screen_capture
