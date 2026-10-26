#pragma once

#include "core/ShaderMacroDefinitions.hpp"

#include <Enums.hpp>

#include <filesystem>

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

    bool isValid() const
    {
        return stage != Shader_Stage::none && !path.empty();
    }

    Shader_Stage stage = Shader_Stage::none;
    std::filesystem::path path;
    core::ShaderMacroDefinitions macroDefinitions;
};
} // namespace star::command::shader
