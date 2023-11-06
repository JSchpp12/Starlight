#pragma once 

#include "StarMaterialMesh.hpp"
#include "CastHelpers.hpp"
#include "StarMaterial.hpp"
#include "StarDevice.hpp"
#include "StarDescriptors.hpp"
#include "Vertex.hpp"

#include <vulkan/vulkan.hpp>

#include <array>

namespace star{
	class StarMesh {
	public:
		StarMesh(std::unique_ptr<std::vector<Vertex>> vertices, std::unique_ptr<std::vector<uint32_t>> indices, 
			std::unique_ptr<StarMaterial> material) : 
			vertices(std::move(vertices)), indices(std::move(indices)),
			material(std::move(material)) {
			//calculate tangents for all provided verts and indices
			for (int i = 0; i < this->indices->size() - 3; i += 3) {
				//go through each group of 3 verts -- assume they are triangles
				//apply needed calculations

				std::array<Vertex*, 3> triVerts{
					&this->vertices->at(this->indices->at(i)),
					&this->vertices->at(this->indices->at(i + 1)),
					&this->vertices->at(this->indices->at(i + 2))
				};
				calculateTangentSpaceVectors(triVerts);
			}
		};

		virtual void prepRender(StarDevice& device);

		virtual void recordCommands(vk::CommandBuffer& commandBuffer, vk::PipelineLayout& pipelineLayout, int swapChainImageIndex, uint32_t vb_start, uint32_t ib_start);

		std::vector<Vertex>& getVertices() { return *this->vertices; }
		std::vector<uint32_t>& getIndices() { return *this->indices; }
		StarMaterial& getMaterial() { return *this->material; }

	protected:
		std::unique_ptr<std::vector<Vertex>> vertices; 
		std::unique_ptr<std::vector<uint32_t>> indices; 
		std::unique_ptr<StarMaterial> material; 

		/// <summary>
		/// Calculate the tangent and bitangent vectors for each vertex in the triangle
		/// </summary>
		void calculateTangentSpaceVectors(std::array<Vertex*,3> verts) {
			glm::vec3 tangent = glm::vec3(), bitangent = glm::vec3();
			glm::vec3 edge1 = verts[1]->pos - verts[0]->pos;
			glm::vec3 edge2 = verts[2]->pos - verts[0]->pos;
			glm::vec2 deltaUV1 = verts[1]->texCoord - verts[0]->texCoord;
			glm::vec2 deltaUV2 = verts[2]->texCoord - verts[0]->texCoord;

			float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

			tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
			tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
			tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

			bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
			bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
			bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

			verts[0]->aTangent = tangent;
			verts[0]->aBitangent = bitangent;
			verts[1]->aTangent = tangent;
			verts[1]->aBitangent = bitangent;
			verts[2]->aTangent = tangent;
			verts[2]->aBitangent = bitangent;
		}
	};
}