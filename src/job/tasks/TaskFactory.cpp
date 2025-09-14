#include "tasks/TaskFactory.hpp"

#include "Compiler.hpp"
#include "StarShader.hpp"
#include "complete_tasks/CompleteTask.hpp"
#include "complete_tasks/TaskFactory.hpp"

#include "FileHelpers.hpp"

#include <memory>

star::job::tasks::Task<> star::job::tasks::task_factory::createPrintTask(std::string message)
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

void star::job::tasks::task_factory::ExecuteBuildPipeline(void *p){
    auto *payload = static_cast<PipelineBuildPayload *>(p); 

    payload->pipeline->prepRender(payload->device, *payload->deps); 
}

std::optional<star::job::complete_tasks::CompleteTask<>> star::job::tasks::task_factory::CreateBuildComplete(void *payload){
    auto *p = static_cast<PipelineBuildPayload *>(payload); 

    assert(p->pipeline && "Pipeline not a valid object in the payload"); 
    
    return std::make_optional<complete_tasks::CompleteTask<>>(complete_tasks::task_factory::CreateBuildPipelineComplete(p->handleID, std::move(p->pipeline))); 
}

star::job::tasks::Task<> star::job::tasks::task_factory::CreateBuildPipeline(
    vk::Device device, Handle handle, StarPipeline::RenderResourceDependencies deps, StarPipeline pipeline)
{
    return job::tasks::Task<>::Builder<PipelineBuildPayload>()
    .setPayload(PipelineBuildPayload{
        .device = std::move(device), 
        .handleID = handle.getID(),
        .deps = std::make_unique<star::StarPipeline::RenderResourceDependencies>(std::move(deps)), 
        .pipeline = std::make_unique<StarPipeline>(std::move(pipeline))
    })
    .setExecute(&ExecuteBuildPipeline)
    .setCreateCompleteTaskFunction(&CreateBuildComplete)
    .build();
}

#pragma endregion BuildPipeline

std::optional<star::job::complete_tasks::CompleteTask<>> star::job::tasks::task_factory::CreateCompileComplete(void *p)
{
    auto *data = static_cast<CompileShaderPayload *>(p);

    auto complete = job::complete_tasks::task_factory::CreateShaderCompileComplete(
        data->handleID, std::move(data->finalizedShaderObject), std::move(data->compiledShaderCode));
    data->compiledShaderCode = nullptr;

    return std::make_optional<star::job::complete_tasks::CompleteTask<>>(std::move(complete));
}

void star::job::tasks::task_factory::ExecuteCompileShader(void *p)
{
    auto *data = static_cast<CompileShaderPayload *>(p);

    data->finalizedShaderObject = std::make_unique<StarShader>(data->path, data->stage);
    std::cout << "Beginning compile shader: " << data->path << std::endl;
    data->compiledShaderCode = star::Compiler::compile(data->path, true);
}

star::job::tasks::Task<> star::job::tasks::task_factory::CreateCompileShader(const std::string &fileName,
                                                                             const star::Shader_Stage &stage,
                                                                             const Handle &shaderHandle)
{
    return star::job::tasks::Task<>::Builder<CompileShaderPayload>()
        .setPayload(CompileShaderPayload{
            .path = fileName,
            .stage = stage,
            .handleID = shaderHandle.getID(),
        })
        .setExecute(&ExecuteCompileShader)
        .setCreateCompleteTaskFunction(&CreateCompileComplete)
        .build();
}