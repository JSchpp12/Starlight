#pragma once 

#include "GeometryHelpers.hpp"
#include "CastHelpers.hpp"
#include "StarCommandBuffer.hpp"
#include "StarDevice.hpp"
#include "StarDescriptorBuilders.hpp"
#include "StarMaterialMesh.hpp"
#include "StarMaterial.hpp"
#include "Vertex.hpp"
#include "CastHelpers.hpp"

#include <vulkan/vulkan.hpp>

#include <array>

namespace star{
	class StarMesh {
	public:
		StarMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, 
			std::shared_ptr<StarMaterial> material, bool hasAdjacenciesPacked) : 
			material(std::move(material)), hasAdjacenciesPacked(hasAdjacenciesPacked), 
			triangular(indices.size() % 3 == 0), numVerts(CastHelpers::size_t_to_unsigned_int(vertices.size())), 
			numInds(CastHelpers::size_t_to_unsigned_int(indices.size())) {

			calcBoundingBox(vertices, this->aaboundingBoxBounds[1], this->aaboundingBoxBounds[0]);

		};

		StarMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
			std::shared_ptr<StarMaterial> material, const glm::vec3& boundBoxMinCoord,
			const glm::vec3& boundBoxMaxCoord, bool packAdjacencies = false) :
			material(std::move(material)), hasAdjacenciesPacked(packAdjacencies), 
			triangular(indices.size() % 3 == 0), aaboundingBoxBounds{ boundBoxMinCoord, boundBoxMaxCoord },
			numVerts(CastHelpers::size_t_to_unsigned_int(vertices.size())),
			numInds(CastHelpers::size_t_to_unsigned_int(indices.size()))
		{
		};

		virtual void prepRender(StarDevice& device);

		StarMaterial& getMaterial() { return *this->material; }
		bool hasAdjacentVertsPacked() const { return this->hasAdjacenciesPacked; }
		bool isTriangular() const { return this->triangular; }
		std::array<glm::vec3, 2> getBoundingBoxCoords() const {
			return std::array<glm::vec3, 2>{
				this->aaboundingBoxBounds[0],
				this->aaboundingBoxBounds[1]
			};
		};

		uint32_t getNumVerts() const { return this->numVerts; }
		uint32_t getNumIndices() const { return this->numInds; }

	protected:
		bool hasAdjacenciesPacked = false; 
		bool triangular = false; 
		glm::vec3 aaboundingBoxBounds[2];
		std::shared_ptr<StarMaterial> material; 
		uint32_t numVerts=0, numInds=0; 

		static void calcBoundingBox(const std::vector<Vertex>& verts, glm::vec3& upperBoundingBoxCoord, glm::vec3& lowerBoundingBoxCoord) {
			glm::vec3 max{}, min{};

			//calcualte bounding box info 
			for (int i = 1; i < verts.size(); i++) {
				if (verts.at(i).pos.x < min.x)
					min.x = verts.at(i).pos.x;
				if (verts.at(i).pos.y < min.y)
					min.y = verts.at(i).pos.y;
				if (verts.at(i).pos.z < min.z)
					min.z = verts.at(i).pos.z;

				if (verts.at(i).pos.x > max.x)
					max.x = verts.at(i).pos.x;
				if (verts.at(i).pos.y > max.y)
					max.y = verts.at(i).pos.y; 
				if (verts.at(i).pos.z > max.z)
					max.z = verts.at(i).pos.z;
			}

			lowerBoundingBoxCoord = min;
			upperBoundingBoxCoord = max; 
		}
	};
}