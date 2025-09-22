#include "job/tasks/task_factory/CompileShader.hpp"

#include "job/complete_tasks/TaskFactory.hpp"

namespace star::job::tasks::task_factory::compile_shader
{
std::optional<star::job::complete_tasks::CompleteTask<>> CreateComplete(void *p)
{
    auto *data = static_cast<CompileShaderPayload *>(p);

    auto complete = job::complete_tasks::task_factory::CreateShaderCompileComplete(
        data->handleID, std::move(data->finalizedShaderObject), std::move(data->compiledShaderCode));
    data->compiledShaderCode = nullptr;

    return std::make_optional<star::job::complete_tasks::CompleteTask<>>(std::move(complete));
}

void Execute(void *p)
{
    auto *data = static_cast<CompileShaderPayload *>(p);

    data->finalizedShaderObject = std::make_unique<StarShader>(data->path, data->stage);
    std::cout << "Beginning compile shader: " << data->path << std::endl;
    data->compiledShaderCode = data->compiler->compile(data->path, true); 
}

star::job::tasks::Task<> Create(const std::string &fileName, const star::Shader_Stage &stage,
                                const Handle &shaderHandle, std::unique_ptr<Compiler> compiler)
{
    return star::job::tasks::Task<>::Builder<CompileShaderPayload>()
        .setPayload(CompileShaderPayload{
            .path = fileName,
            .stage = stage,
            .handleID = shaderHandle.getID(),
            .compiler = std::move(compiler)
        })
        .setExecute(&Execute)
        .setCreateCompleteTaskFunction(&CreateComplete)
        .build();
}
} // namespace star::job::tasks::task_factory::compile_shader