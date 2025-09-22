#pragma once

#include "Handle.hpp"
#include "StarShader.hpp"
#include "job/tasks/Task.hpp"
#include "job/complete_tasks/CompleteTask.hpp"

#include <memory>

namespace star::job::tasks::task_factory::compile_shader
{
struct CompileShaderPayload
{
    std::string path;
    star::Shader_Stage stage;
    uint32_t handleID;
    std::unique_ptr<Compiler> compiler = nullptr; 
    std::unique_ptr<StarShader> finalizedShaderObject = nullptr;
    std::unique_ptr<std::vector<uint32_t>> compiledShaderCode = nullptr;
};

std::optional<star::job::complete_tasks::CompleteTask<>> CreateComplete(void *p); 

void Execute(void *p);

star::job::tasks::Task<> Create(const std::string &fileName, const star::Shader_Stage &stage,
                                             const Handle &shaderHandle, std::unique_ptr<Compiler> compiler);


} // namespace star::job::tasks::task_factory::compile_shader