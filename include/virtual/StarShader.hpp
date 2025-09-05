#pragma once

#include "Compiler.hpp"

#include "spirv_reflect.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace star
{
class StarShader
{
  public:
    StarShader() = default;
    StarShader(const std::string &path, const star::Shader_Stage &stage) : path(path), stage(stage)
    {
    }
    ~StarShader() = default;
    StarShader(const StarShader &) = default;
    StarShader &operator=(const StarShader &) = default;
    StarShader(StarShader &&) = default;
    StarShader &operator=(StarShader &&) = default;
    
    star::Shader_Stage getStage()
    {
        return this->stage;
    }

    std::string getPath()
    {
        return this->path;
    }

  protected:
    std::string path = "";
    star::Shader_Stage stage = star::Shader_Stage::none;
};
} // namespace star