#pragma once

#include "VertColorMaterial.hpp"
#include "HeightDisplacementMaterial.hpp"
#include "StarMesh.hpp"
#include "RuntimeUpdateTexture.hpp"
#include "StarObject.hpp"
#include "Vertex.hpp"
#include "Color.hpp"
#include "StarGraphicsPipeline.hpp"
#include "ConfigFile.hpp"

#include <glm/glm.hpp>

#include <memory>

namespace star {
	/// <summary>
	/// Grid object which will programmatically generate vertices as needed.
	/// </summary>
	class Grid : public StarObject {
	public:
		virtual ~Grid(); 
		
		Grid(int vertX, int vertY);

		Grid(int vertX, int vertY, std::shared_ptr<StarMaterial> material);

		//std::optional<glm::vec3> getWorldCoordsWhereRayIntersectsMe(glm::vec3 tail, glm::vec3 head);

		std::optional<glm::vec2> getXYCoordsWhereRayIntersectsMe(glm::vec3 tail, glm::vec3 head);

		int getSizeX() { return this->vertX; }
		int getSizeY() { return this->vertY; }
	protected:

		int width = 0.5;

		int vertX = 0, vertY = 0;

		glm::vec3 getCenter();

		void loadGeometry(std::unique_ptr<std::vector<Vertex>>& verts, std::unique_ptr<std::vector<uint32_t>>& inds); 

		virtual std::unordered_map<star::Shader_Stage, StarShader> getShaders() override;
	};
}