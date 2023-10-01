#pragma once 

#include "Compiler.hpp"

#include <vector> 
#include <iostream>
#include <memory> 
#include <string>

namespace star {
    class StarShader {
    public:
        StarShader(const std::string& path); 
        virtual ~StarShader() {};

        std::unique_ptr<std::vector<uint32_t>> compiledCode;

    protected:
        std::string path;

    private:

    };
}