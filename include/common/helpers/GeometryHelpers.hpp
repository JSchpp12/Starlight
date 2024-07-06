#pragma once 

#include "Vertex.hpp"

#include <vector>
#include <unordered_map>
#include <string>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <array>

namespace star {
	class GeometryHelpers {
	public:
		static void packTriangleAdjacency(std::vector<Vertex>& verts, std::vector<uint32_t>& indices);

		static void calculateAndApplyVertTangents(std::vector<Vertex>& verts, std::vector<uint32_t>& indices);

		static void calculateAxisAlignedBoundingBox(const glm::vec3 lowerBound, const glm::vec3 upperBound,
			std::vector<Vertex>& vertList, std::vector<uint32_t>& indicesList,
			bool lineList = false);
	private:
		struct EdgeTracker {
			std::pair<uint32_t, uint32_t> verts; 
			EdgeTracker(const uint32_t f, const uint32_t s) 
				: verts(std::pair<uint32_t, uint32_t>(f, s)) {};

			bool operator==(const EdgeTracker& f) const {
				return ((this->verts.first == f.verts.first && this->verts.second == f.verts.second)
					|| (this->verts.first == f.verts.second && this->verts.second == f.verts.first)); 
			}
		};

		struct TriangleTracker {
			EdgeTracker edges[3]; 
			TriangleTracker* neighbors[3]{ nullptr, nullptr, nullptr };

			TriangleTracker(const EdgeTracker f, const EdgeTracker s, const EdgeTracker t)
				: edges{ f, s, t } {}; 

			uint32_t getOppositeVertIndex(uint32_t vertIndex) {
				return 0;
			}

			void addNeighbor(const int edgeIndex, TriangleTracker& neighbor){
				assert(edgeIndex < 3 && "An edge index outside of acceptable range requested. A triangle can only have 3 edges");
				this->neighbors[edgeIndex] = &neighbor; 
			}
		};

		static void calcTangentSpaceVectors(std::array<Vertex*, 3>& triVerts);
	};
}