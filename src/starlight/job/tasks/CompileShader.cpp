#include "job/tasks/CompileShader.hpp"

#include "job/complete_tasks/CompileShader.hpp"
#include "logging/LoggingFactory.hpp"

#include <exception>
#include <vector>

namespace star::job::tasks::compile_shader
{
std::optional<star::job::complete_tasks::CompleteTask> CreateComplete(void *p)
{
    auto *data = static_cast<CompileShaderPayload *>(p);

    return std::make_optional<star::job::complete_tasks::CompleteTask>(
        job::complete_tasks::compile_shader::CreateShaderCompileComplete(data->handleID,
                                                                         std::move(data->shaderFuture)));
}

void Execute(void *p)
{
    auto *data = static_cast<CompileShaderPayload *>(p);

    {
        const std::string msg = "Beginning compile shader: " + data->path.string();
        core::logging::log(boost::log::trivial::info, msg);
    }

    try
    {
        // use the default shader compiler
        star::Compiler compiler{data->macroDefinitions};
        const std::vector<uint32_t> compiledCode = compiler.compile(data->path.string(), data->compileDebug);

        core::logging::log(boost::log::trivial::info, "Done");

        data->shaderPromise.set_value(StarShader{data->device, data->stage, compiledCode});
    }
    catch (...)
    {
        // Keep the failure on the main thread instead of letting it unwind
        // through the worker thread.
        data->shaderPromise.set_exception(std::current_exception());
    }
}

CompileShaderTask Create(std::filesystem::path fileName, const star::Shader_Stage &stage, const Handle &shaderHandle,
                         vk::Device device, star::core::ShaderMacroDefinitions macroDefinitions, bool compileDebug)
{
    auto promise = std::promise<StarShader>{};
    auto future = promise.get_future();

    return CompileShaderTask::Builder<CompileShaderPayload>()
        .setPayload(CompileShaderPayload{.path = fileName,
                                         .macroDefinitions = std::move(macroDefinitions),
                                         .stage = stage,
                                         .handleID = shaderHandle.getID(),
                                         .compileDebug = compileDebug,
                                         .device = device,
                                         .shaderPromise = std::move(promise),
                                         .shaderFuture = std::move(future)})
        .setExecute(&Execute)
        .setCreateCompleteTaskFunction(&CreateComplete)
        .build();
}
} // namespace star::job::tasks::compile_shader
