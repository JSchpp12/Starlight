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

void star::GeometryHelpers::calculateAndApplyVertTangents(std::vector<star::Vertex>& verts, std::vector<uint32_t>& indices)
{
	//calculate tangents for all provided verts and indices
	for (int i = 0; i < indices.size()  - 3; i += 3) {
		//go through each group of 3 verts -- assume they are triangles
		//apply needed calculations

		std::array<Vertex*, 3> triVerts{
			&verts.at(indices.at(i)),
			&verts.at(indices.at(i + 1)),
			&verts.at(indices.at(i + 2))
		};
		calcTangentSpaceVectors(triVerts);
	}
}

void star::GeometryHelpers::calculateAxisAlignedBoundingBox(const glm::vec3 lowerBound, const glm::vec3 upperBound, std::vector<star::Vertex>& vertList, std::vector<uint32_t>& indicesList, bool lineList)
{
	glm::vec3 center{
		(upperBound.x + lowerBound.x) / 2,
		(upperBound.y + lowerBound.y) / 2,
		(upperBound.z + lowerBound.z) / 2
	};

	vertList = std::vector<Vertex>{
		Vertex{lowerBound, lowerBound - center},
		Vertex{glm::vec3{upperBound.x, lowerBound.y, lowerBound.z}, glm::vec3{upperBound.x, lowerBound.y, lowerBound.z} - center},
		Vertex{glm::vec3{upperBound.x, lowerBound.y, upperBound.z}, glm::vec3{lowerBound.x, upperBound.y, lowerBound.z} - center},
		Vertex{glm::vec3{lowerBound.x, lowerBound.y, upperBound.z}, glm::vec3{upperBound.x, upperBound.y, lowerBound.z} - center},
		Vertex{glm::vec3{lowerBound.x, upperBound.y, lowerBound.z}, glm::vec3{lowerBound.x, upperBound.y, lowerBound.z} - center},
		Vertex{glm::vec3{upperBound.x, upperBound.y, lowerBound.z}, glm::vec3{upperBound.x, upperBound.y, lowerBound.z} - center},
		Vertex{upperBound, upperBound - center},
		Vertex{glm::vec3{lowerBound.x, upperBound.y ,upperBound.z}, glm::vec3{lowerBound.x, upperBound.y, upperBound.z} - center}
	};
	
	if (lineList)
		indicesList = std::vector<uint32_t>{
		//base
		0, 1, 
		1, 2, 
		2, 3, 
		3, 0, 
		//middle
		0, 4, 
		1, 5,
		2, 6,
		3, 7,
		//top
		4, 5, 
		5, 6, 
		6, 7, 
		7, 4
	};
	else
		indicesList = std::vector<uint32_t>{
			//L
			1, 0, 4, 
			1, 4, 5, 
			//Back
			2, 1, 5, 
			2, 5, 6, 
			//R
			3, 2, 6, 
			3, 6, 7, 
			//Front
			0, 3, 7, 
			0, 7, 4, 
			//Bottom
			1, 2, 3, 
			1, 3, 0,
			//Top
			4, 7, 6, 
			4, 6, 5
		};
}

void star::GeometryHelpers::calcTangentSpaceVectors(std::array<star::Vertex*, 3>& triVerts)
{
	glm::vec3 tangent = glm::vec3(), bitangent = glm::vec3();
	glm::vec3 edge1 = triVerts[1]->pos - triVerts[0]->pos;
	glm::vec3 edge2 = triVerts[2]->pos - triVerts[0]->pos;
	glm::vec2 deltaUV1 = triVerts[1]->texCoord - triVerts[0]->texCoord;
	glm::vec2 deltaUV2 = triVerts[2]->texCoord - triVerts[0]->texCoord;

	float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

	bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

	triVerts[0]->aTangent = tangent;
	triVerts[0]->aBitangent = bitangent;
	triVerts[1]->aTangent = tangent;
	triVerts[1]->aBitangent = bitangent;
	triVerts[2]->aTangent = tangent;
	triVerts[2]->aBitangent = bitangent;
}
