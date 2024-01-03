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

		std::unordered_map<star::Shader_Stage, StarShader> getShaders() override;

	protected:
		Square(); 

		void load(); 

		std::pair<std::unique_ptr<StarBuffer>, std::unique_ptr<StarBuffer>> loadGeometryStagingBuffers(StarDevice& device, Handle& primaryVertBuffer, Handle& primaryIndexBuffer) override;

	};
}
