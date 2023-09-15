#pragma once 
#include "Vertex.hpp"
#include "Handle.hpp"

#include <glm/glm.hpp>

#include <array>

namespace star {
struct Triangle {
	Triangle() = default;
	Triangle(std::array<Vertex, 3> v) : vertices(v) {
		this->calculateTangentSpaceVectors();
	}

	std::array<Vertex, 3> vertices;
	Handle material;

	/// <summary>
	/// Calculate the tangent and bitangent vectors for each vertex in the triangle
	/// </summary>
	void calculateTangentSpaceVectors() {
		glm::vec3 tangent = glm::vec3(), bitangent = glm::vec3();
		glm::vec3 edge1 = vertices[1].pos - vertices[0].pos;
		glm::vec3 edge2 = vertices[2].pos - vertices[0].pos;
		glm::vec2 deltaUV1 = vertices[1].texCoord - vertices[0].texCoord;
		glm::vec2 deltaUV2 = vertices[2].texCoord - vertices[0].texCoord;

		float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

		tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
		tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
		tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

		bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
		bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
		bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

		vertices[0].aTangent = tangent;
		vertices[0].aBitangent = bitangent;
		vertices[1].aTangent = tangent;
		vertices[1].aBitangent = bitangent;
		vertices[2].aTangent = tangent;
		vertices[2].aBitangent = bitangent;
	}
};
}