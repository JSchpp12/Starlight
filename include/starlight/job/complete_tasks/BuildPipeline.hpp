#pragma once

#include "StarPipeline.hpp"
#include "starlight/job/complete_tasks/CompleteTask.hpp"

namespace star::job::complete_tasks
{

struct PipelineBuildCompletePayload
{
    uint32_t handleID = 0;
    star::StarPipeline pipeline;
};

void ExecuteBuildPipelineComplete(void *device, void *taskSystem, void *eventBus, void *graphicsManagers,
                                  void *payload);

complete_tasks::CompleteTask CreateBuildPipelineComplete(uint32_t handleID, StarPipeline pipeline);

} // namespace star::job::complete_tasks