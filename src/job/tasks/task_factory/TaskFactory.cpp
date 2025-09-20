#include "tasks/task_factory/TaskFactory.hpp"

#include "Compiler.hpp"
#include "StarShader.hpp"
#include "complete_tasks/CompleteTask.hpp"
#include "complete_tasks/TaskFactory.hpp"

#include "FileHelpers.hpp"

#include <boost/filesystem.hpp>

#include <memory>

namespace star::job::tasks::task_factory
{
star::job::tasks::Task<> createPrintTask(std::string message)
{
    return star::job::tasks::Task<>::Builder<PrintPayload>()
        .setPayload(PrintPayload(std::move(message)))
        .setExecute([](void *p) {
            auto *data = static_cast<PrintPayload *>(p);
            std::cout << data->message << std::endl;
        })
        .build();
}

#pragma region BuildPipeline

void ExecuteBuildPipeline(void *p)
{
    auto *payload = static_cast<PipelineBuildPayload *>(p);

    payload->pipeline->prepRender(payload->device, *payload->deps);
}

std::optional<star::job::complete_tasks::CompleteTask<>> CreateBuildComplete(void *payload)
{
    auto *p = static_cast<PipelineBuildPayload *>(payload);

    assert(p->pipeline && "Pipeline not a valid object in the payload");

    return std::make_optional<complete_tasks::CompleteTask<>>(
        complete_tasks::task_factory::CreateBuildPipelineComplete(p->handleID, std::move(p->pipeline)));
}

star::job::tasks::Task<> CreateBuildPipeline(vk::Device device, Handle handle,
                                             StarPipeline::RenderResourceDependencies deps, StarPipeline pipeline)
{
    return job::tasks::Task<>::Builder<PipelineBuildPayload>()
        .setPayload(PipelineBuildPayload{
            .device = std::move(device),
            .handleID = handle.getID(),
            .deps = std::make_unique<star::StarPipeline::RenderResourceDependencies>(std::move(deps)),
            .pipeline = std::make_unique<StarPipeline>(std::move(pipeline))})
        .setExecute(&ExecuteBuildPipeline)
        .setCreateCompleteTaskFunction(&CreateBuildComplete)
        .build();
}

#pragma endregion BuildPipeline

} // namespace star::job::tasks::task_factory