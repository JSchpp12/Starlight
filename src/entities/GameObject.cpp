#include "GameObject.hpp"

namespace star {
	std::vector<std::unique_ptr<Mesh>> GameObject::loadFromFile(const std::string path) {
		/* Load Object From File */
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), materialFile.c_str(), true)) {
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
		std::vector<std::unique_ptr<Mesh>> meshes(shapes.size());
		std::vector<Handle> objectMaterialHandles;
		std::vector<std::reference_wrapper<Material>> objectMaterials;
		std::unique_ptr<std::vector<Triangle>> triangles;
		tinyobj::material_t* currMaterial = nullptr;
		std::unique_ptr<Material> objectMaterial;

		if (materials.size() > 0) {
			//create needed materials
			for (size_t i = 0; i < materials.size(); i++) {
				currMaterial = &materials.at(i);
				auto& builder = Materials::Builder(*this)
					.setDiffuse(glm::vec4{
						currMaterial->diffuse[0],
						currMaterial->diffuse[1],
						currMaterial->diffuse[2],
						1.0f })
						.setSpecular(glm::vec4{
								currMaterial->specular[0],
								currMaterial->specular[1],
								currMaterial->specular[2],
								1.0f })
								.setShinyCoefficient(currMaterial->shininess);
				//check if need to override texture 
				if (matPropOverride != nullptr && matPropOverride->baseColorTexture != nullptr) {
					builder.setTexture(*matPropOverride->baseColorTexture);
				}
				else if (currMaterial->diffuse_texname != "") {
					loadMaterialTexture = this->textureManager.addResource(texturePath + FileHelpers::GetFileNameWithExtension(currMaterial->diffuse_texname));
					builder.setTexture(loadMaterialTexture);
				}

				//apply maps 
				if (currMaterial->bump_texname != "") {
					auto bumpTexture = this->textureManager.addResource(texturePath + FileHelpers::GetFileNameWithExtension(currMaterial->bump_texname));
					builder.setBumpMap(bumpTexture);
				}
				else {
					builder.setBumpMap(Handle::getDefault());
				}

				objectMaterialHandles.push_back(builder.build());
				//record material to avoid multiple fetches
				objectMaterials.push_back(this->materialManager.resource(objectMaterialHandles.at(i)));
			}
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

					//check for color override
					if (!overrideColor) {
						triangleVerticies[i].color = {
							attrib.colors[3 * indicies[dIndex].vertex_index + 0],
							attrib.colors[3 * indicies[dIndex].vertex_index + 1],
							attrib.colors[3 * indicies[dIndex].vertex_index + 2],
						};
					}
					else {
						triangleVerticies[i].color = {
							overrideColor->r,
							overrideColor->g,
							overrideColor->b
						};
					}

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
					if (loadMaterials && shape.mesh.material_ids.at(faceIndex) != -1) {
						//use the overridden material if provided, otherwise use the prop from mtl file
						triangleVerticies[i].matAmbient = (matPropOverride != nullptr && matPropOverride->ambient != nullptr) ? *matPropOverride->ambient : objectMaterials.at(shape.mesh.material_ids.at(faceIndex)).get().ambient;
						triangleVerticies[i].matDiffuse = (matPropOverride != nullptr && matPropOverride->diffuse != nullptr) ? *matPropOverride->diffuse : objectMaterials.at(shape.mesh.material_ids.at(faceIndex)).get().diffuse;
						triangleVerticies[i].matSpecular = (matPropOverride != nullptr && matPropOverride->specular != nullptr) ? *matPropOverride->specular : objectMaterials.at(shape.mesh.material_ids.at(faceIndex)).get().specular;
						triangleVerticies[i].matShininess = (matPropOverride != nullptr && matPropOverride->shiny != nullptr) ? *matPropOverride->shiny : objectMaterials.at(shape.mesh.material_ids.at(faceIndex)).get().shinyCoefficient;
					}
					else {
						//use either the overriden material property or the passed material
						triangleVerticies[i].matAmbient = (matPropOverride != nullptr && matPropOverride->ambient != nullptr) ? *matPropOverride->ambient : this->materialManager.resource(selectedMaterial).ambient;
						triangleVerticies[i].matDiffuse = (matPropOverride != nullptr && matPropOverride->diffuse != nullptr) ? *matPropOverride->diffuse : this->materialManager.resource(selectedMaterial).diffuse;
						triangleVerticies[i].matSpecular = (matPropOverride != nullptr && matPropOverride->specular != nullptr) ? *matPropOverride->specular : this->materialManager.resource(selectedMaterial).specular;
						triangleVerticies[i].matShininess = (matPropOverride != nullptr && matPropOverride->shiny != nullptr) ? *matPropOverride->shiny : this->materialManager.resource(selectedMaterial).shinyCoefficient;
					}
				}

				triangles->at(faceIndex) = Triangle(triangleVerticies);
			}

			if (shape.mesh.material_ids.at(shapeCounter) != -1) {
				//apply material from files to mesh -- will ignore passed values 
				meshes.at(shapeCounter) = std::move(Mesh::Builder()
					.setTriangles(std::move(triangles))
					.setMaterial(objectMaterialHandles.at(shape.mesh.material_ids[0]))
					.build());
			}
			else {
				meshes.at(shapeCounter) = std::move(Mesh::Builder()
					.setTriangles(std::move(triangles))
					.setMaterial(selectedMaterial)
					.build());
			}
			shapeCounter++;
		}

		std::cout << "Loaded: " << pathToFile << std::endl;

		return std::move(meshes);
	}
}