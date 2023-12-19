#include "GeometryHelpers.hpp"

void star::GeometryHelpers::packTriangleAdjacency(std::vector<Vertex>& verts, std::vector<uint32_t>& indices)
{
	std::vector<uint32_t> neighborIndices; 

	std::vector<uint32_t> uniqueIndicies; 
	std::unordered_map<glm::vec3, uint32_t> vertIndexMap; 
	std::vector<Vertex> finalizedVerts;
	uint32_t uniqueIndexCount = 0; 

	//ensure unique verts
	for (int i = 0; i < indices.size(); i++) {
		//check if vert is in another triangle

		glm::vec3 loc = verts.at(indices.at(i)).pos; 
		if (vertIndexMap.find(loc) != vertIndexMap.end()) 
			uniqueIndicies.push_back(vertIndexMap.at(loc)); 
		else {
			vertIndexMap.insert(std::pair<glm::vec3, uint32_t>(loc, uniqueIndexCount));
			finalizedVerts.push_back(verts[i]); 
			uniqueIndicies.push_back(uniqueIndexCount); 
			uniqueIndexCount++; 
		}
	}

	std::vector<TriangleTracker> triangles; 

	//create triangles
	for (int i = 0; i <= uniqueIndicies.size()-3; i+=3) {
		triangles.push_back(TriangleTracker{
			EdgeTracker(uniqueIndicies[i], uniqueIndicies[i + 1]),
			EdgeTracker(uniqueIndicies[i + 1], uniqueIndicies[i + 2]),
			EdgeTracker(uniqueIndicies[i + 2], uniqueIndicies[i])
		});
	}

	for (int i = 0; i < triangles.size(); i++) {
		for (int j = 0; j < i; j++) {
			bool isNeighbor = false; 
			for (int k = 0; k < 3; k++) {
				EdgeTracker& f = triangles[i].edges[k];
				for (int m = 0; m < 3; m++) {
					EdgeTracker& s = triangles[j].edges[m];
					if (f == s) {
						isNeighbor = true;
						triangles[i].addNeighbor(k, triangles[j]);
						triangles[j].addNeighbor(m, triangles[i]);
						break;
					}
				}
				if (isNeighbor)
					break;
			}
		}
	}

	//pack adjacency information
	std::vector<uint32_t> adjInds; 

	for (int i = 0; i < triangles.size(); i++) {
		for (int j = 0; j < 3; j++) {
			EdgeTracker& curEdge = triangles[i].edges[j]; 
			adjInds.push_back(curEdge.verts.first); 
			adjInds.push_back(triangles[i].neighbors[j]->getOppositeVertIndex(curEdge.verts.first)); 
		}
	}

	
	indices = adjInds; 
	verts = finalizedVerts; 
}
