#pragma once 

#include "VertColorMaterial.hpp"
#include "StarObject.hpp"
#include "StarGraphicsPipeline.hpp"
#include "StarMesh.hpp"
#include "ConfigFile.hpp"

namespace star {
	class Square : public star::StarObject {
	public:
		static std::unique_ptr<Square> New(); 

		// Inherited via StarObject
		std::unique_ptr<StarPipeline> buildPipeline(StarDevice& device, vk::Extent2D swapChainExtent, vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass) override;

		std::unordered_map<star::Shader_Stage, StarShader> getShaders() override;

	protected:
		Square(); 

		void load(); 

	};
}
