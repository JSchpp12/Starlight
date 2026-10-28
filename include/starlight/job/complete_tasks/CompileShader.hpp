#pragma once

#include "StarShader.hpp"
#include "job/complete_tasks/CompleteTask.hpp"
#include "job/tasks/CompileShaderResult.hpp"

#include <cstdint>
#include <future>
#include <string_view>

namespace star::job::complete_tasks::compile_shader
{
using CompileResult = star::job::tasks::compile_shader::CompileResult;
using CompileFailure = star::job::tasks::compile_shader::CompileFailure;
using CompileFailureKind = star::job::tasks::compile_shader::CompileFailureKind;

constexpr std::string_view CompileShaderTypeName = "star::job::complete_tasks::compile_shader";

struct CompileCompletePayload
{
    uint32_t handleID = 0;
    std::future<CompileResult> shaderFuture;
};

void ProcessPipelinesWhichAreNowReadyForBuild(void *device, void *taskSystem, void *graphicsManagers);

void ExecuteShaderCompileComplete(void *device, void *taskSystem, void *eventBus, void *graphicsManagers,
                                  void *payload);

star::job::complete_tasks::CompleteTask CreateShaderCompileComplete(uint32_t handleID,
                                                                    std::future<CompileResult> shaderFuture);
} // namespace star::job::complete_tasks::compile_shader
