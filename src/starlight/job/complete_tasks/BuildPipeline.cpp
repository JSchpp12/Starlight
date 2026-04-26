#include "starlight/job/complete_tasks/BuildPipeline.hpp"

#include "starlight/core/device/managers/GraphicsContainer.hpp"
#include "starlight/event/PipelineReady.hpp"

static void SignalPipelineReady(star::common::EventBus &evtBus, star::Handle registration)
{
    star::event::PipelineReady evt{std::move(registration)};

    evtBus.emit(evt); 
}

void star::job::complete_tasks::ExecuteBuildPipelineComplete(void *device, void *taskSystem, void *eventBus,
                                                             void *graphicsManagers, void *payload)
{
    auto *gm = static_cast<core::device::manager::GraphicsContainer *>(graphicsManagers);

    auto *p = static_cast<PipelineBuildCompletePayload *>(payload);

    auto handle = Handle{
        .type = common::HandleTypeRegistry::instance().getTypeGuaranteedExist(common::special_types::PipelineTypeName),
        .id = p->handleID};

    std::cout << "Pipeline at [" << p->handleID << "] is ready" << std::endl;

    gm->pipelineManager->get(handle)->request.pipeline = std::move(*p->pipeline);

    auto *evtBus = static_cast<star::common::EventBus *>(eventBus); 
    SignalPipelineReady(*evtBus, std::move(handle)); 
}

star::job::complete_tasks::CompleteTask star::job::complete_tasks::CreateBuildPipelineComplete(
    uint32_t handleID, std::unique_ptr<StarPipeline> pipeline)
{
    return CompleteTask::Builder<PipelineBuildCompletePayload>()
        .setPayload(PipelineBuildCompletePayload{.handleID = std::move(handleID), .pipeline = std::move(pipeline)})
        .setExecuteFunction(&ExecuteBuildPipelineComplete)
        .build();
}