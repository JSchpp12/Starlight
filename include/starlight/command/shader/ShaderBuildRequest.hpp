#pragma once

#include <Compiler.hpp>
#include <Enums.hpp>

#include <cstddef>
#include <filesystem>
#include <string>
#include <utility>

namespace star::command::shader
{
/// The engine only ever combines a small, fixed set of stages into a single
/// pipeline, so commands can store their shader descriptions inline instead of
/// on the heap.
inline constexpr std::size_t MaxShadersPerPipeline = 4;

/// Inert description of a shader that should be compiled. This is a command-side
/// value type so that commands can describe shader work without depending on the
/// device managers.
struct ShaderBuildRequest
{
    ShaderBuildRequest() = default;
    ShaderBuildRequest(Shader_Stage stage, std::filesystem::path path, Compiler compiler = Compiler())
        : stage(stage), path(std::move(path)), compiler(std::move(compiler))
    {
    }

    bool isValid() const
    {
        return stage != Shader_Stage::none && !path.empty();
    }

    std::string pathString() const
    {
        return path.string();
    }

    Shader_Stage stage = Shader_Stage::none;
    std::filesystem::path path;
    Compiler compiler;
};
} // namespace star::command::shader
