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
        StarShader(const std::string& path); 
        ~StarShader() = default;

        /// <summary>
        /// Compile the shader to SPIR-V 
        /// </summary>
        /// <returns></returns>
        std::unique_ptr<std::vector<uint32_t>> compile();

    protected:
        std::string path;

    private:

    };
}