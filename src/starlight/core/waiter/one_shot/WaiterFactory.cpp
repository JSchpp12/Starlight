#include "starlight/core/waiter/one_shot/WaiterFactory.hpp"

#include "starlight/core/waiter/one_shot/GenericEvent.hpp"
#include "starlight/core/waiter/one_shot/detail/CachedPipelinePayload.hpp"
#include "starlight/event/PipelineReady.hpp"

#include <cassert>

namespace star::core::waiter::one_shot
{
namespace on_build_pipeline
{

void BuildSetCachedPipeline(star::common::EventBus &evtBus, star::core::device::manager::Pipeline &pipeManager,
                            star::Handle registration, vk::Pipeline *cachedPipelineToWrite)
{
    // build waiter with proper payload
    star::core::waiter::one_shot::GenericEvent<CachedPipelinePayload, star::event::PipelineReady>::Builder(evtBus)
        .setPayload(CachedPipelinePayload{.targetPipelineRegistration = std::move(registration),
                                          .pipelineToSet = cachedPipelineToWrite,
                                          .pipelineManager = &pipeManager})
        .build();
}
} // namespace on_build_pipeline
} // namespace star::core::waiter::one_shot