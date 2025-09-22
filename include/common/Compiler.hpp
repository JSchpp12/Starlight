/*
 * This class contains the implementation of a online shader compiler.
 */

#pragma once

#include "FileHelpers.hpp"
#include "shaderc/shaderc.hpp"

#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace star
{

class Compiler
{
  public:
    Compiler() = default; 
    Compiler(std::string precompilerMacros) : m_precompilerMacros(std::move(precompilerMacros)){}

    // compile provided shader to spirv
    std::unique_ptr<std::vector<uint32_t>> compile(const std::string &pathToFile, bool optimize);

  private:
    static bool compileDebug;
    std::string m_precompilerMacros;

    // get the shaderc stage flag for the shader stage
    static shaderc_shader_kind getShaderCStageFlag(const std::string &pathToFile);

    // preprocess shader code before compilation
    std::string preprocessShader(shaderc::Compiler &compiler, shaderc::CompileOptions options,
                                        const std::string &sourceName, shaderc_shader_kind stage,
                                        const std::string &source);

    shaderc::CompileOptions getCompileOptions(const std::string &filePath);
};

} // namespace star