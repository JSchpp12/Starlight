#pragma once

#include "Compiler.hpp"
#include "StarShader.hpp"
#include "job/complete_tasks/CompleteTask.hpp"
#include "job/tasks/CompileShaderResult.hpp"
#include "job/tasks/Task.hpp"

#include "starlight/util/io/PathResolver.hpp"

#include <star_common/Handle.hpp>

#include <filesystem>
#include <future>
#include <memory>
#include <string>
#include <string_view>
#include <vulkan/vulkan.hpp>

namespace star::job::tasks::compile_shader
{
constexpr std::string_view CompileShaderTypeName = "star::job::tasks::compile_shader";

/// Whether debug info is generated for compiled shaders by default. Derived
/// from NDEBUG so debug builds opt in and release builds opt out.
#ifdef NDEBUG
constexpr bool defaultCompileDebug = false;
#else
constexpr bool defaultCompileDebug = true;
#endif

struct CompileShaderPayload
{
    std::filesystem::path path;
    star::core::ShaderMacroDefinitions macroDefinitions;
    star::io::PathResolver pathResolver;
    star::Shader_Stage stage;
    uint32_t handleID;
    bool compileDebug = defaultCompileDebug;
    vk::Device device = VK_NULL_HANDLE;
    std::promise<CompileResult> shaderPromise;
    std::future<CompileResult> shaderFuture;
};

using CompileShaderTask = star::job::tasks::Task<sizeof(CompileShaderPayload), alignof(CompileShaderPayload)>;

std::optional<star::job::complete_tasks::CompleteTask> CreateComplete(void *p);

void Execute(void *p);

CompileShaderTask Create(std::filesystem::path fileName, const star::Shader_Stage &stage, const Handle &shaderHandle,
                         vk::Device device, star::core::ShaderMacroDefinitions macroDefinitions,
                         star::io::PathResolver pathResolver, bool compileDebug = defaultCompileDebug);

} // namespace star::job::tasks::compile_shader