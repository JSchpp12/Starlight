#include "StarSystemPipeline.hpp"

star::StarSystemPipeline::StarSystemPipeline(std::vector<StarShader> definedShaders)
{
	std::vector<star::Shader_Stage> currentStages; 

	//ensure there are no duplicate shaders and quick check for compatibility
	for (StarShader shader : definedShaders) {
		//check if current shader stage is already part of this pipeline
		bool found = false; 

		for (auto stage : currentStages) {
			if (stage == shader.getStage()) {
				found = true; 
				break;
			}
		}

		assert(!found && "More than one shader has been requested of the same stage. This is not supported in the same system."); 

		shaders.push_back(shader); 
	}
}
