#pragma once

#include "common/helpers/FileHelpers.hpp"
#include "core/ShaderMacroDefinitions.hpp"
#include "shaderc/shaderc.hpp"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace star
{

class Compiler
{
  public:
    explicit Compiler(core::ShaderMacroDefinitions &macroDefinitions) : m_macroDefinitions(macroDefinitions)
    {
    }

    // compile provided shader to spirv
    std::vector<uint32_t> compile(const std::string &pathToFile, bool compileDebug);

  private:
    core::ShaderMacroDefinitions &m_macroDefinitions;

    // get the shaderc stage flag for the shader stage
    static shaderc_shader_kind getShaderCStageFlag(const std::string &pathToFile);

    // preprocess shader code before compilation
    std::string preprocessShader(shaderc::Compiler &compiler, shaderc::CompileOptions options,
                                 const std::string &sourceName, shaderc_shader_kind stage, const std::string &source);

    shaderc::CompileOptions getCompileOptions(const std::string &filePath, bool compileDebug);
};

} // namespace star