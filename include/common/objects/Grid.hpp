#pragma once

#include "VertColorMaterial.hpp"
#include "HeightDisplacementMaterial.hpp"
#include "StarMesh.hpp"
#include "RuntimeUpdateTexture.hpp"
#include "StarObject.hpp"
#include "Vertex.hpp"
#include "Color.hpp"

#include <glm/glm.hpp>

#include <memory>

namespace star {
	/// <summary>
	/// Grid object which will programmatically generate vertices as needed.
	/// </summary>
	class Grid : public StarObject {
	public:
		Grid(int vertX, int vertY);

		void updateTexture(std::vector<int> locsX, std::vector<int> locY, const std::vector<Color> newColor);

		std::optional<glm::vec3> getWorldCoordsWhereRayIntersectsMe(glm::vec3 tail, glm::vec3 head);

		std::optional<glm::vec2> getXYCoordsWhereRayIntersectsMe(glm::vec3 tail, glm::vec3 head);

		const std::vector<std::unique_ptr<StarMesh>>& getMeshes() override { return this->meshes; };
		Color getTexColorAt(int x, int y) { return this->material.getTexture().getRawData()->at(x).at(y); }
		int getSizeX() { return this->vertX; }
		int getSizeY() { return this->vertY; }
	protected:
		int width = 0.5;

		int vertX = 0, vertY = 0;
		HeightDisplacementMaterial material;
		std::vector<std::unique_ptr<StarMesh>> meshes;
		std::unique_ptr<RuntimeUpdateTexture> displacementTexture;

		glm::vec3 getCenter();
	};
}