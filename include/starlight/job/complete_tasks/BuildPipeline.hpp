#pragma once

#include "StarPipeline.hpp"
#include "starlight/job/complete_tasks/CompleteTask.hpp"

namespace star::job::complete_tasks
{

struct PipelineBuildCompletePayload
{
    uint32_t handleID = 0;
    std::unique_ptr<star::StarPipeline> pipeline = nullptr;
};

void ExecuteBuildPipelineComplete(void *device, void *taskSystem, void *eventBus, void *graphicsManagers,
                                  void *payload);

complete_tasks::CompleteTask CreateBuildPipelineComplete(uint32_t handleID, std::unique_ptr<StarPipeline> pipeline);

}