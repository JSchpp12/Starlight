#pragma once

#include "starlight/core/device/managers/Pipeline.hpp"

#include <star_common/IEvent.hpp>
#include <star_common/Handle.hpp>

#include <vulkan/vulkan.hpp>

namespace star::core::waiter::one_shot
{
struct CachedPipelinePayload
{
    star::Handle targetPipelineRegistration;
    vk::Pipeline *pipelineToSet{nullptr};
    const star::core::device::manager::Pipeline *pipelineManager{nullptr};

    void operator()(const star::common::IEvent &e, bool &keepAlive);
};

} // namespace star::core::waiter::one_shot
