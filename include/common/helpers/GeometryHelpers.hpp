#pragma once 

#include "Vertex.hpp"

#include <vector>
#include <unordered_map>
#include <string>
#include <glm/gtx/hash.hpp>

namespace star {
	class GeometryHelpers {
	public:
		static void packTriangleAdjacency(std::vector<Vertex>& verts, std::vector<uint32_t>& indices);

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
			TriangleTracker* neighbors[3];

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
	};
}