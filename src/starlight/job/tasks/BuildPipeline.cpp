#include "job/tasks/BuildPipeline.hpp"

#include "starlight/job/complete_tasks/BuildPipeline.hpp"

namespace star::job::tasks::build_pipeline
{

void ExecuteBuildPipeline(void *p)
{
    auto *payload = static_cast<PipelineBuildPayload *>(p);

    payload->data->built = payload->data->provider.build(payload->data->device, payload->data->deps);
}

std::optional<star::job::complete_tasks::CompleteTask> CreateBuildComplete(void *p)
{
    auto *payload = static_cast<PipelineBuildPayload *>(p);

    assert(payload->data->built.isRenderReady() && "Pipeline was not built by ExecuteBuildPipeline");

    return std::make_optional<complete_tasks::CompleteTask>(
        job::complete_tasks::CreateBuildPipelineComplete(payload->data->handleID, std::move(payload->data->built)));
}

BuildPipelineTask CreateBuildPipeline(vk::Device device, Handle handle,
                                      star::StarPipeline::RenderResourceDependencies buildDeps,
                                      PipelineProvider provider)
{
    return BuildPipelineTask::Builder<PipelineBuildPayload>()
        .setPayload(PipelineBuildPayload{std::make_unique<PipelineData>(std::move(buildDeps), std::move(provider),
                                                                        star::StarPipeline(), device, handle.getID())})
        .setExecute(&ExecuteBuildPipeline)
        .setCreateCompleteTaskFunction(&CreateBuildComplete)
        .build();
}
} // namespace star::job::tasks::build_pipeline