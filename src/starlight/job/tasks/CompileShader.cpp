#include "job/tasks/CompileShader.hpp"

#include "job/complete_tasks/CompileShader.hpp"
#include "logging/LoggingFactory.hpp"

#include <exception>
#include <utility>
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

    CompileResult result = [&]() -> CompileResult
    {
        // Resolve on the worker thread: the caller only had to hand over a
        // value copy of the resolver, never the IOService (which belongs to
        // the main thread).
        const auto resolvedPath = data->pathResolver.resolve(data->path, star::io::PathKind::file);
        if (!resolvedPath)
        {
            return std::unexpected(CompileFailure{CompileFailureKind::pathResolution, resolvedPath.error().toString()});
        }

        core::logging::log(boost::log::trivial::info, "Resolved shader path: " + resolvedPath->string());

        try
        {
            // use the default shader compiler
            star::Compiler compiler{data->macroDefinitions};
            const std::vector<uint32_t> compiledCode = compiler.compile(resolvedPath->string(), data->compileDebug);

            core::logging::log(boost::log::trivial::info, "Done");

            return StarShader{data->device, data->stage, compiledCode};
        }
        catch (const std::exception &e)
        {
            return std::unexpected(CompileFailure{CompileFailureKind::compilation, e.what()});
        }
        catch (...)
        {
            return std::unexpected(CompileFailure{CompileFailureKind::compilation, "unknown compilation error"});
        }
    }();

    // Report the outcome as a value; the complete task owns the failure policy.
    data->shaderPromise.set_value(std::move(result));
}

CompileShaderTask Create(std::filesystem::path fileName, const star::Shader_Stage &stage, const Handle &shaderHandle,
                         vk::Device device, star::core::ShaderMacroDefinitions macroDefinitions,
                         star::io::PathResolver pathResolver, bool compileDebug)
{
    auto promise = std::promise<CompileResult>{};
    auto future = promise.get_future();

    return CompileShaderTask::Builder<CompileShaderPayload>()
        .setPayload(CompileShaderPayload{.path = fileName,
                                         .macroDefinitions = std::move(macroDefinitions),
                                         .pathResolver = std::move(pathResolver),
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
