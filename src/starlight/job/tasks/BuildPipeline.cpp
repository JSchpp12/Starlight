#include "job/tasks/BuildPipeline.hpp"

#include "starlight/job/complete_tasks/BuildPipeline.hpp"

#include <exception>
#include <utility>

namespace star::job::tasks::build_pipeline
{

void ExecuteBuildPipeline(void *p)
{
    auto *payload = static_cast<PipelineBuildPayload *>(p);

    try
    {
        star::StarPipeline built =
            payload->data->provider.createPipeline(payload->data->device, payload->data->deps);
        payload->data->pipelinePromise.set_value(std::move(built));
    }
    catch (const std::exception &e)
    {
        payload->data->pipelinePromise.set_value(
            std::unexpected(star::core::ResourceCreationError{e.what()}));
    }
    catch (...)
    {
        payload->data->pipelinePromise.set_value(
            std::unexpected(star::core::ResourceCreationError{"unknown pipeline build failure"}));
    }
}

std::optional<star::job::complete_tasks::CompleteTask> CreateBuildComplete(void *p)
{
    auto *payload = static_cast<PipelineBuildPayload *>(p);

    return std::make_optional<complete_tasks::CompleteTask>(
        job::complete_tasks::CreateBuildPipelineComplete(payload->data->handleID));
}

BuildPipelineTask CreateBuildPipeline(
    vk::Device device, Handle handle, star::StarPipeline::RenderResourceDependencies buildDeps,
    PipelineProvider provider,
    std::promise<star::core::device::manager::PendingResult<star::StarPipeline>> pipelinePromise)
{
    return BuildPipelineTask::Builder<PipelineBuildPayload>()
        .setPayload(PipelineBuildPayload{std::make_unique<PipelineData>(PipelineData{
            .deps = std::move(buildDeps),
            .provider = std::move(provider),
            .device = device,
            .handleID = handle.getID(),
            .pipelinePromise = std::move(pipelinePromise)})})
        .setExecute(&ExecuteBuildPipeline)
        .setCreateCompleteTaskFunction(&CreateBuildComplete)
        .build();
}
} // namespace star::job::tasks::build_pipeline