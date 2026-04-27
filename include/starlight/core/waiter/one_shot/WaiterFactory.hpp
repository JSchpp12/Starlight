#pragma once

#include "starlight/core/device/managers/Pipeline.hpp"

#include <star_common/Handle.hpp>
#include <star_common/IEvent.hpp>

namespace star::core::waiter::one_shot
{
namespace on_build_pipeline
{

void BuildSetCachedPipeline(star::common::EventBus &evtBus, star::core::device::manager::Pipeline &pipeManager,
                            star::Handle registration, vk::Pipeline *cachedPipelineToWrite);

} // namespace on_build_pipeline

} // namespace star::core::waiter::one_shot