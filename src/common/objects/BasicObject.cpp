#include "BasicObject.hpp"

std::unique_ptr<star::BasicObject> star::BasicObject::New(std::string objPath)
{
	return std::unique_ptr<BasicObject>(new BasicObject(objPath));
}

std::vector<std::unique_ptr<star::StarMesh>> star::BasicObject::loadMeshes()
{
	return std::move(loadFromFile(objectFilePath)); 
}

std::vector<std::unique_ptr<star::StarMesh>> star::BasicObject::loadFromFile(const std::string objectFilePath)
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
	std::vector<std::unique_ptr<BumpMaterial>> objectMaterials;
	std::vector<std::unique_ptr<StarMesh>> meshes(shapes.size());
	std::unique_ptr<std::vector<Triangle>> triangles;
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

			objectMaterials.push_back(std::unique_ptr<BumpMaterial>( new BumpMaterial(glm::vec4(1.0),
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
						)));
		}

		//need to scale object so that it fits on screen
		//combine all attributes into a single object 
		std::array<Vertex, 3> triangleVerticies;
		int dIndex = 0;
		for (const auto& shape : shapes) {
			triangleCounter = 0;
			threeCounter = 0;
			counter = 0;

			//tinyobj ensures three verticies per triangle  -- assuming unique verticies 
			verticies = std::make_unique<std::vector<Vertex>>(shape.mesh.indices.size());
			const std::vector<tinyobj::index_t>& indicies = shape.mesh.indices;
			triangles = std::make_unique<std::vector<Triangle>>(shape.mesh.material_ids.size());

			for (size_t faceIndex = 0; faceIndex < shape.mesh.material_ids.size(); faceIndex++) {
				for (int i = 0; i < 3; i++) {
					dIndex = (3 * faceIndex) + i;
					triangleVerticies[i].pos = {
						attrib.vertices[3 * indicies[dIndex].vertex_index + 0],
						attrib.vertices[3 * indicies[dIndex].vertex_index + 1],
						attrib.vertices[3 * indicies[dIndex].vertex_index + 2],
					};

					triangleVerticies[i].color = {
						attrib.colors[3 * indicies[dIndex].vertex_index + 0],
						attrib.colors[3 * indicies[dIndex].vertex_index + 1],
						attrib.colors[3 * indicies[dIndex].vertex_index + 2],
					};

					if (attrib.normals.size() > 0) {
						triangleVerticies[i].normal = {
							attrib.normals[3 * indicies[dIndex].normal_index + 0],
							attrib.normals[3 * indicies[dIndex].normal_index + 1],
							attrib.normals[3 * indicies[dIndex].normal_index + 2],
						};
					}

					triangleVerticies[i].texCoord = {
						attrib.texcoords[2 * indicies[dIndex].texcoord_index + 0],
						1.0f - attrib.texcoords[2 * indicies[dIndex].texcoord_index + 1]
					};
					if (shape.mesh.material_ids.at(faceIndex) != -1) {
						//use the overridden material if provided, otherwise use the prop from mtl file
						triangleVerticies[i].matAmbient = objectMaterials.at(shape.mesh.material_ids.at(faceIndex))->ambient;
						triangleVerticies[i].matDiffuse = objectMaterials.at(shape.mesh.material_ids.at(faceIndex))->diffuse;
						triangleVerticies[i].matSpecular = objectMaterials.at(shape.mesh.material_ids.at(faceIndex))->specular;
						triangleVerticies[i].matShininess = objectMaterials.at(shape.mesh.material_ids.at(faceIndex))->shinyCoefficient;
					}
				}

				triangles->at(faceIndex) = Triangle(triangleVerticies);
			}

			if (shape.mesh.material_ids.at(shapeCounter) != -1) {
				//apply material from files to mesh -- will ignore passed values 
				meshes.at(shapeCounter) = std::make_unique<StarMesh>(std::move(triangles), std::move(objectMaterials.at(shape.mesh.material_ids[0])));
			}
			shapeCounter++;
		}
	}

	return std::move(meshes);
}