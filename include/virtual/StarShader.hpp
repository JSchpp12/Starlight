#pragma once 

#include "Compiler.hpp"

#include <vector> 
#include <iostream>
#include <memory> 
#include <string>

namespace star {
    class StarShader {
    public:
        std::string path;
        StarShader(const std::string& path); 
        virtual ~StarShader() {};

        std::unique_ptr<std::vector<uint32_t>> compiledCode;

    protected:

    private:

    };
}