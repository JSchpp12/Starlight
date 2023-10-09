#include "Square.hpp"

std::vector<std::unique_ptr<star::StarMesh>> star::Square::loadMeshes()
{
	std::unique_ptr<std::vector<star::Vertex>> verts = std::unique_ptr<std::vector<star::Vertex>>(new std::vector<star::Vertex>{
		star::Vertex{
			glm::vec3{-0.5f, -0.5f, 0.0f},	//position
			glm::vec3{0.0f, 0.0f, -1.0f},	//normal - posy
			glm::vec3{0.0f, 1.0f, 0.0f}		//color
		},
		star::Vertex{
			glm::vec3{0.5f, -0.5f, 0.0f},	//position
			glm::vec3{0.0f, 0.0f, -1.0f},	//normal - posy
			glm::vec3{0.0f, 1.0f, 0.0f}		//color
		},
		star::Vertex{
			glm::vec3{0.5f, 0.5f, 0.0f},	//position
			glm::vec3{0.0f, 0.0f, 0.0f},	//normal - posy
			glm::vec3{1.0f, 0.0f, 0.0f}		//color
		},
		star::Vertex{
			glm::vec3{-0.5f, 0.5f, 0.0f},	//position
			glm::vec3{0.0f, 0.0f, -1.0f},	//normal - posy
			glm::vec3{0.0f, 1.0f, 0.0f}		//color
		},
	});
	std::unique_ptr<std::vector<uint32_t>> inds = std::unique_ptr<std::vector<uint32_t>>(new std::vector<uint32_t>{
		0,3,2,0,2,1
	});

	std::unique_ptr<star::VertColorMaterial> material = std::unique_ptr<star::VertColorMaterial>(new star::VertColorMaterial());
	auto meshes = std::vector<std::unique_ptr<star::StarMesh>>();
	meshes.emplace_back(std::unique_ptr<star::StarMesh>(new star::StarMesh(std::move(verts), std::move(inds), std::move(material))));

	return std::move(meshes);
}
