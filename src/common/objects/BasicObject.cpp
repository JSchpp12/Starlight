#include "BasicObject.hpp"

std::unique_ptr<star::BasicObject> star::BasicObject::New(std::string objPath)
{
	return std::unique_ptr<BasicObject>(new BasicObject(objPath));
}

star::BasicObject::BasicObject(std::string objectFilePath)
{
	loadFromFile(objectFilePath);
}

void star::BasicObject::loadFromFile(const std::string objectFilePath)
{
	std::string texturePath = FileHelpers::GetBaseFileDirectory(objectFilePath);
	std::string materialFile = FileHelpers::GetBaseFileDirectory(objectFilePath);

	/* Load Object From File */
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objectFilePath.c_str(), materialFile.c_str(), true)) {
		throw std::runtime_error(warn + err);
	}
	if (warn != "") {
		std::cout << "An error occurred while loading obj file" << std::endl;
		std::cout << warn << std::endl;
		std::cout << "Loading will continue..." << std::endl;
	}

	float maxVal = 0;
	size_t counter = 0;
	size_t shapeCounter = 0;
	size_t materialIndex = 0;
	size_t triangleCounter = 0;
	size_t threeCounter = 0;

	Vertex* currVertex = nullptr;
	Handle loadMaterialTexture;
	std::unique_ptr<std::vector<Vertex>> verticies;
	std::unique_ptr<std::vector<uint32_t>> indicies;
	std::unique_ptr<std::vector<std::pair<unsigned int, unsigned int>>> sortedIds;
	std::vector<std::unique_ptr<StarMesh>> meshes(shapes.size());
	tinyobj::material_t* currMaterial = nullptr;
	std::unique_ptr<StarMaterial> objectMaterial;


	if (materials.size() > 0) {
		//create needed materials
		for (size_t i = 0; i < materials.size(); i++) {
			currMaterial = &materials.at(i);
			std::unique_ptr<Texture> texture;
			std::unique_ptr<Texture> bumpMap;

			if (currMaterial->diffuse_texname != "") {
				texture = std::unique_ptr<Texture>(new Texture(texturePath + FileHelpers::GetFileNameWithExtension(currMaterial->diffuse_texname)));
			}

			//apply maps 
			if (currMaterial->bump_texname != "") {
				bumpMap = std::unique_ptr<Texture>(new Texture(texturePath + FileHelpers::GetFileNameWithExtension(currMaterial->bump_texname)));
			}

			this->materials.emplace_back(BumpMaterial(glm::vec4(1.0),
				glm::vec4(1.0),
				glm::vec4(1.0),
				glm::vec4{
					currMaterial->diffuse[0],
					currMaterial->diffuse[1],
					currMaterial->diffuse[2],
					1.0f },
					glm::vec4{
						currMaterial->specular[0],
						currMaterial->specular[1],
						currMaterial->specular[2],
						1.0f },
						currMaterial->shininess,
						std::move(texture),
						std::move(bumpMap)
						));
		}

		//need to scale object so that it fits on screen
		//combine all attributes into a single object 
		int dIndex = 0;
		for (const auto& shape : shapes) {
			triangleCounter = 0;
			threeCounter = 0;
			counter = 0;

			//tinyobj ensures three verticies per triangle  -- assuming unique vertices 
			const std::vector<tinyobj::index_t>& indicies = shape.mesh.indices;
			auto fullInd = std::make_unique<std::vector<uint32_t>>(shape.mesh.indices.size());
			auto vertices = std::make_unique<std::vector<Vertex>>(shape.mesh.indices.size());
			size_t vertCounter = 0;
			for (size_t faceIndex = 0; faceIndex < shape.mesh.material_ids.size(); faceIndex++) {
				for (int i = 0; i < 3; i++) {
					dIndex = (3 * faceIndex) + i;
					auto newVertex = Vertex(); 
					newVertex.pos = glm::vec3{
						attrib.vertices[3 * indicies[dIndex].vertex_index + 0],
						attrib.vertices[3 * indicies[dIndex].vertex_index + 1],
						attrib.vertices[3 * indicies[dIndex].vertex_index + 2]
					};
					newVertex.color = glm::vec3{
						attrib.colors[3 * indicies[dIndex].vertex_index + 0],
						attrib.colors[3 * indicies[dIndex].vertex_index + 1],
						attrib.colors[3 * indicies[dIndex].vertex_index + 2],
					};

					if (attrib.normals.size() > 0) {
						newVertex.normal = {
							attrib.normals[3 * indicies[dIndex].normal_index + 0],
							attrib.normals[3 * indicies[dIndex].normal_index + 1],
							attrib.normals[3 * indicies[dIndex].normal_index + 2],
						};
					}

					newVertex.texCoord = {
						attrib.texcoords[2 * indicies[dIndex].texcoord_index + 0],
						1.0f - attrib.texcoords[2 * indicies[dIndex].texcoord_index + 1]
					};

					if (shape.mesh.material_ids.at(faceIndex) != -1) {
						//use the overridden material if provided, otherwise use the prop from mtl file
						newVertex.matAmbient = this->materials.at(shape.mesh.material_ids.at(faceIndex)).ambient;
						newVertex.matDiffuse = this->materials.at(shape.mesh.material_ids.at(faceIndex)).diffuse;
						newVertex.matSpecular = this->materials.at(shape.mesh.material_ids.at(faceIndex)).specular;
						newVertex.matShininess = this->materials.at(shape.mesh.material_ids.at(faceIndex)).shinyCoefficient;
					}

					vertices->at(vertCounter) = newVertex;
					fullInd->at(vertCounter) = star::CastHelpers::size_t_to_unsigned_int(vertCounter); 
					vertCounter++; 
				};
			}

			if (shape.mesh.material_ids.at(shapeCounter) != -1) {
				//apply material from files to mesh -- will ignore passed values 
				meshes.at(shapeCounter) = std::unique_ptr<StarMesh>(new StarMesh(std::move(vertices), std::move(fullInd), this->materials.at(shape.mesh.material_ids[0])));
			}
			shapeCounter++;
		}
	}

	this->meshes = std::move(meshes); 
}
