#pragma once

#include "StarShader.hpp"
#include "job/complete_tasks/CompleteTask.hpp"
#include "job/tasks/Task.hpp"
#include <starlight/common/Handle.hpp>

#include <memory>

namespace star::job::tasks::compile_shader
{
constexpr std::string_view CompileShaderTypeName = "star::job::tasks::compile_shader";

struct CompileShaderPayload
{
    std::string path;
    star::Shader_Stage stage;
    uint32_t handleID;
    std::unique_ptr<Compiler> compiler = nullptr;
    std::unique_ptr<StarShader> finalizedShaderObject = nullptr;
    std::shared_ptr<std::vector<uint32_t>> compiledShaderCode = nullptr;
};

using CompileShaderTask = star::job::tasks::Task<sizeof(CompileShaderPayload), alignof(CompileShaderPayload)>;

std::optional<star::job::complete_tasks::CompleteTask> CreateComplete(void *p);

void Execute(void *p);

CompileShaderTask Create(const std::string &fileName, const star::Shader_Stage &stage, const Handle &shaderHandle,
                         Compiler compiler);

} // namespace star::job::tasks::compile_shader