#include "StarShader.hpp"

namespace star {
StarShader::StarShader(const std::string& path) : 
    path(path){
    //generateDescriptorList(); 
}

//void StarShader::generateDescriptorList() {
//    SpvReflectShaderModule shaderModule;
//    SpvReflectResult result = spvReflectCreateShaderModule(4 * compiledCode->size(), compiledCode->data(), &shaderModule);
//
//    assert(result == SPV_REFLECT_RESULT_SUCCESS);
//}

std::unique_ptr<std::vector<uint32_t>> StarShader::compile() {
    return Compiler::compile(path, true); 
}

}