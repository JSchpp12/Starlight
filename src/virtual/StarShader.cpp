#include "StarShader.hpp"

namespace star {
StarShader::StarShader(const std::string& path) : 
    path(path),
    compiledCode(Compiler::compile(path, true)){
    //generateDescriptorList(); 
}

//void Shader::generateDescriptorList() {
//    SpvReflectShaderModule shaderModule; 
//    SpvReflectResult result = spvReflectCreateShaderModule(4 * compiledCode->size(), compiledCode->data(), &shaderModule); 

//    assert(result == SPV_REFLECT_RESULT_SUCCESS); 
//}

}