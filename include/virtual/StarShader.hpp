#pragma once 

#include "Compiler.hpp"

#include "spirv_reflect.h"

#include <vector> 
#include <iostream>
#include <memory> 
#include <string>

namespace star {
    class StarShader {
    public:
        StarShader(const std::string& path, star::Shader_Stage stage); 
        ~StarShader() = default;

        /// <summary>
        /// Compile the shader to SPIR-V 
        /// </summary>
        /// <returns></returns>
        std::unique_ptr<std::vector<uint32_t>> compile();

        star::Shader_Stage getStage() { return this->stage; }

        std::string getPath() { return this->path;  }
    protected:
        std::string path;
        star::Shader_Stage stage;

    };
}