#include "device/managers/Shader.hpp"

#include "core/logging/LoggingFactory.hpp"
#include "job/tasks/TaskFactory.hpp"

#include <exception>
#include <utility>

namespace star::core::device::manager
{
using star::job::tasks::compile_shader::CompileFailure;
using star::job::tasks::compile_shader::CompileFailureKind;
using star::job::tasks::compile_shader::CompileResult;

CompileResult ShaderCompileFuture::take() noexcept
{
    if (!future.valid())
        return std::unexpected(
            CompileFailure{CompileFailureKind::incomplete, "shader compile did not complete"});

    try
    {
        return future.get();
    }
    catch (const std::exception &e)
    {
        return std::unexpected(CompileFailure{CompileFailureKind::incomplete, e.what()});
    }
    catch (...)
    {
        return std::unexpected(
            CompileFailure{CompileFailureKind::incomplete, "unknown shader compile failure"});
    }
}

bool ShaderRecord::isReady() const
{
    if (compiled.has_value())
    {
        if (!compiled->isReady())
            return false;

        resolve();
    }

    return request.shader.isValid();
}

void ShaderRecord::resolve() const
{
    if (!compiled.has_value() || !compiled->isReady())
        return;

    CompileResult result = compiled->take();
    compiled.reset();

    if (result.has_value())
        request.shader = std::move(*result);
    else
        core::logging::warning("Shader compile failed: ", result.error().message);
}

void ShaderRecord::waitForReady() const
{
    if (!compiled.has_value())
        return;

    compiled->wait();
    resolve();
}

void ShaderRecord::cleanupRender(core::device::StarDevice &device)
{
    if (compiled.has_value())
    {
        // An unpublishing compile is owned by its producer; it must publish (or be destroyed) before teardown.
        if (!compiled->isReady())
            return;

        resolve();
    }

    request.shader.cleanupRender(device.getVulkanDevice());
}

void Shader::submitTask(device::StarDevice &device, const Handle &handle, job::TaskManager &taskSystem,
                        common::EventBus &eventBus, ShaderRecord *storedRecord)
{
    (void)eventBus;

    auto promise = std::promise<CompileResult>{};
    storedRecord->compiled.emplace(ShaderCompileFuture{promise.get_future()});

    taskSystem.submitTask(job::tasks::compile_shader::Create(
                              std::move(storedRecord->request.path), storedRecord->request.shader.getStage(), handle,
                              device.getVulkanDevice(), std::move(storedRecord->request.macroDefinitions),
                              std::move(storedRecord->request.pathResolver), std::move(promise)),
                          job::tasks::compile_shader::CompileShaderTypeName);
}
} // namespace star::core::device::manager
