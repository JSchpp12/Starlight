#pragma once

#include "StarShader.hpp"

#include <expected>
#include <string>

namespace star::job::tasks::compile_shader
{

/// @brief Why a shader could not be produced. The compile worker reports this as avalue; the complete task (main
/// thread) decides what to do with it.
enum class CompileFailureKind
{
    pathResolution,
    compilation,
    incomplete
};

struct CompileFailure
{
    CompileFailureKind kind = CompileFailureKind::compilation;
    std::string message;
};

using CompileResult = std::expected<StarShader, CompileFailure>;
} // namespace star::job::tasks::compile_shader
