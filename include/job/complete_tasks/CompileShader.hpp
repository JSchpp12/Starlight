#pragma once

#include "job/complete_tasks/CompleteTask.hpp"
#include "StarShader.hpp"

#include <vector>

namespace star::job::complete_tasks::compile_shader
{
struct CompileCompletePayload
{
    uint32_t handleID;
    std::unique_ptr<star::StarShader> finalizedShaderObject = nullptr;
    std::unique_ptr<std::vector<uint32_t>> compiledShaderCode = nullptr;
};


void ProcessPipelinesWhichAreNowReadyForBuild(void *device, void *taskSystem, void *graphicsManagers);

void ExecuteShaderCompileComplete(void *device, void *taskSystem, void *eventBus, void *graphicsManagers,
                                  void *payload);

star::job::complete_tasks::CompleteTask CreateShaderCompileComplete(
    uint32_t handleID, std::unique_ptr<StarShader> finalizedShaderObject,
    std::unique_ptr<std::vector<uint32_t>> finalizedCompiledShader);
}