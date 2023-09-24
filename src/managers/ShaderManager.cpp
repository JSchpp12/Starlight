#include "ShaderManager.hpp"

namespace star {

void ShaderManager::setDefault(const std::string & defaultVert, const std::string & defaultFrag)
{
    defaultVertShader = this->addResource(defaultVert, std::make_unique<StarShader>(defaultVert));
    defaultFragShader = this->addResource(defaultFrag, std::make_unique<StarShader>(defaultFrag));
}

ShaderManager::~ShaderManager() { }

StarShader& ShaderManager::resource(const Handle& resourceHandle) {
    if (resourceHandle.type == Handle_Type::defaultHandle) {
        if (resourceHandle.shaderStage.has_value() && resourceHandle.shaderStage.value() == Shader_Stage::vertex) {
            return this->FileResourceManager<StarShader>::resource(this->defaultVertShader);
        }
        else if (resourceHandle.shaderStage.has_value() && resourceHandle.shaderStage.value() == Shader_Stage::fragment) {
            return this->FileResourceManager<StarShader>::resource(this->defaultFragShader);
        }
        else {
            throw std::runtime_error("Unexpected default shader requested");
        }
    }
    else {
        return this->FileResourceManager<StarShader>::resource(resourceHandle);
    }
}

Handle ShaderManager::createAppropriateHandle() {
    Handle newHandle;
    newHandle.type = Handle_Type::shader;
    return newHandle;
}
}
