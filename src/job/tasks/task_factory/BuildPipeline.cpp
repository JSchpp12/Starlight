#include "job/tasks/task_factory/BuildPipeline.hpp"

#include "complete_tasks/TaskFactory.hpp"

namespace star::job::tasks::task_factory::build_pipeline
{

void ExecuteBuildPipeline(void *p)
{
    auto *payload = static_cast<PipelineBuildPayload *>(p);

    payload->pipeline->prepRender(payload->device, *payload->deps);
}

std::optional<star::job::complete_tasks::CompleteTask> CreateBuildComplete(void *payload)
{
    auto *p = static_cast<PipelineBuildPayload *>(payload);

    assert(p->pipeline && "Pipeline not a valid object in the payload");

    return std::make_optional<complete_tasks::CompleteTask>(
        job::complete_tasks::task_factory::CreateBuildPipelineComplete(p->handleID, std::move(p->pipeline)));
}

BuildPipelineTask CreateBuildPipeline(vk::Device device, Handle handle, StarPipeline::RenderResourceDependencies deps,
                                      StarPipeline pipeline)
{
    return BuildPipelineTask::Builder<PipelineBuildPayload>()
        .setPayload(PipelineBuildPayload{
            .device = std::move(device),
            .handleID = handle.getID(),
            .deps = std::make_unique<star::StarPipeline::RenderResourceDependencies>(std::move(deps)),
            .pipeline = std::make_unique<StarPipeline>(std::move(pipeline))})
        .setExecute(&ExecuteBuildPipeline)
        .setCreateCompleteTaskFunction(&CreateBuildComplete)
        .build();
}
} // namespace star::job::tasks::task_factory::build_pipeline