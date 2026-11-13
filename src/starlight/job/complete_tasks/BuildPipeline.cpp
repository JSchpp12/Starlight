#include "starlight/job/complete_tasks/BuildPipeline.hpp"

#include "core/device/managers/GraphicsContainer.hpp"
#include "core/logging/LoggingFactory.hpp"
#include "starlight/event/PipelineReady.hpp"

#include <star_common/EventBus.hpp>
#include <star_common/Handle.hpp>
#include <star_common/HandleTypeRegistry.hpp>

#include <iostream>

namespace star::job::complete_tasks
{
namespace
{
void SignalPipelineReady(star::common::EventBus &evtBus, star::Handle registration)
{
    star::event::PipelineReady evt{std::move(registration)};
    evtBus.emit(evt);
}
} // namespace

void ExecuteBuildPipelineComplete(void *device, void *taskSystem, void *eventBus, void *graphicsManagers, void *payload)
{
    (void)device;
    (void)taskSystem;

    auto *gm = static_cast<core::device::manager::GraphicsContainer *>(graphicsManagers);
    auto *eb = static_cast<star::common::EventBus *>(eventBus);
    auto *p = static_cast<PipelineBuildCompletePayload *>(payload);

    auto handle = Handle{
        .type = common::HandleTypeRegistry::instance().getTypeGuaranteedExist(common::special_types::PipelineTypeName),
        .id = p->handleID};

    gm->pipelineManager->waitForReady(handle);

    const auto *record = gm->pipelineManager->get(handle);
    if (record == nullptr || !record->isReady())
    {
        core::logging::error("Pipeline build did not produce a usable pipeline for handle [", p->handleID, "]");
        return;
    }

    std::cout << "Pipeline at [" << p->handleID << "] is ready" << std::endl;
    SignalPipelineReady(*eb, std::move(handle));
}

complete_tasks::CompleteTask CreateBuildPipelineComplete(uint32_t handleID)
{
    return complete_tasks::CompleteTask::Builder<PipelineBuildCompletePayload>()
        .setPayload(PipelineBuildCompletePayload{.handleID = handleID})
        .setExecuteFunction(&ExecuteBuildPipelineComplete)
        .build();
}
} // namespace star::job::complete_tasks
