//#include "SceneBuilder.hpp"
//
//namespace star {
//#pragma region GameObjects
//	SceneBuilder::GameObjects::Builder& SceneBuilder::GameObjects::Builder::setPath(const std::string& path) {
//		this->path = std::make_unique<std::string>(path);
//		return *this;
//	}
//	SceneBuilder::GameObjects::Builder& SceneBuilder::GameObjects::Builder::setPosition(const glm::vec3 position) {
//		this->position = position;
//		return *this;
//	}
//	SceneBuilder::GameObjects::Builder& SceneBuilder::GameObjects::Builder::setColor(const glm::vec4& color) {
//		this->color = &color;
//		return *this;
//	}
//	SceneBuilder::GameObjects::Builder& SceneBuilder::GameObjects::Builder::setScale(const glm::vec3 scale) {
//		this->scale = scale;
//		return *this;
//	}
//	SceneBuilder::GameObjects::Builder& SceneBuilder::GameObjects::Builder::setVertShader(const Handle& vertShader) {
//		this->vertShader = vertShader;
//		return *this;
//	}
//	SceneBuilder::GameObjects::Builder& SceneBuilder::GameObjects::Builder::setFragShader(const Handle& fragShader) {
//		this->fragShader = fragShader;
//		return *this;
//	}
//	SceneBuilder::GameObjects::Builder& SceneBuilder::GameObjects::Builder::setVerticies(const std::vector<glm::vec3>& verticies) {
//		this->verticies = std::make_unique<std::vector<Vertex>>(verticies.size());
//
//		for (auto& vert : verticies) {
//			this->verticies->push_back(Vertex{ vert });
//		}
//		return *this;
//	}
//	SceneBuilder::GameObjects::Builder& SceneBuilder::GameObjects::Builder::setIndicies(const std::vector<uint32_t>& indicies) {
//		this->indicies = std::make_unique<std::vector<uint32_t>>(indicies);
//		return *this;
//	}
//	SceneBuilder::GameObjects::Builder& SceneBuilder::GameObjects::Builder::setTexture(const Handle& texture) {
//		if (!this->matOverride) {
//			this->matOverride = std::make_unique<OverrideMaterialProperties>();
//		}
//		this->matOverride->baseColorTexture = &texture;
//		return *this;
//	}
//	SceneBuilder::GameObjects::Builder& SceneBuilder::GameObjects::Builder::setMaterial(Handle materialHandle) {
//		this->materialHandle = &materialHandle;
//		return *this;
//	}
//	SceneBuilder::GameObjects::Builder& SceneBuilder::GameObjects::Builder::overrideAmbient(const glm::vec3& ambient) {
//		if (!this->matOverride) {
//			this->matOverride = std::make_unique<OverrideMaterialProperties>();
//		}
//		this->matOverride->ambient = &ambient;
//		return *this;
//	}
//	SceneBuilder::GameObjects::Builder& SceneBuilder::GameObjects::Builder::overrideDiffuse(const glm::vec3& diffuse) {
//		if (!this->matOverride) {
//			this->matOverride = std::make_unique<OverrideMaterialProperties>();
//		}
//		this->matOverride->diffuse = &diffuse;
//		return *this;
//	}
//	SceneBuilder::GameObjects::Builder& SceneBuilder::GameObjects::Builder::overrideSpecular(const glm::vec3& specular) {
//		if (!this->matOverride) {
//			this->matOverride = std::make_unique<OverrideMaterialProperties>();
//		}
//		this->matOverride->specular = &specular;
//		return *this;
//	}
//	SceneBuilder::GameObjects::Builder& SceneBuilder::GameObjects::Builder::overrideShiny(const float& shiny) {
//		if (!this->matOverride) {
//			this->matOverride = std::make_unique<OverrideMaterialProperties>();
//		}
//		this->matOverride->shiny = &shiny;
//		return *this;
//	}
//	SceneBuilder::GameObjects::Builder& SceneBuilder::GameObjects::Builder::setMaterialFilePath(const std::string& path) {
//		this->materialFilePath = std::make_unique<std::string>(path);
//		return *this;
//	}
//	SceneBuilder::GameObjects::Builder& SceneBuilder::GameObjects::Builder::setTextureDirectory(const std::string& path) {
//		this->textureDirectory = std::make_unique<std::string>(path);
//		return *this;
//	}
//	Handle SceneBuilder::GameObjects::Builder::build(bool loadMaterials) {
//		if (!this->verticies && !this->indicies && this->path) {
//			return this->sceneBuilder.addObject(*this->path, this->position, this->scale, this->materialHandle, this->vertShader, this->fragShader, loadMaterials, this->materialFilePath.get(), this->textureDirectory.get(), this->color, this->matOverride.get());
//		}
//		else if (this->verticies && this->verticies->size() != 0 && this->indicies && this->indicies->size() != 0) {
//
//			//return this->sceneBuilder.add(std::move(this->verticies), std::move(this->indicies), this->position, this->scale, this->material, this->texture, this->vertShader, this->fragShader);
//		}
//		throw std::runtime_error("Invalid parameters provided to complete build of object");
//	}
//#pragma endregion
//
//	/* Lights */
//	SceneBuilder::Lights::Builder& SceneBuilder::Lights::Builder::setType(const Type::Light& type) {
//		this->type = &type;
//		return *this;
//	}
//	SceneBuilder::Lights::Builder& SceneBuilder::Lights::Builder::setLinkedObject(const Handle& linkedObject) {
//		this->linkedHandle = &linkedObject;
//		return *this;
//	}
//	SceneBuilder::Lights::Builder& SceneBuilder::Lights::Builder::setPosition(const glm::vec3& position) {
//		//TODO: need to handle which object (light/Linked GO) will be used for the position 
//		this->position = &position;
//		return *this;
//	}
//	SceneBuilder::Lights::Builder& SceneBuilder::Lights::Builder::setAmbient(const glm::vec4& ambient) {
//		this->ambient = &ambient;
//		return *this;
//	}
//	SceneBuilder::Lights::Builder& SceneBuilder::Lights::Builder::setDiffuse(const glm::vec4& diffuse) {
//		this->diffuse = &diffuse;
//		return *this;
//	}
//	SceneBuilder::Lights::Builder& SceneBuilder::Lights::Builder::setSpecular(const glm::vec4& specular)
//	{
//		this->specular = &specular;
//		return *this;
//	}
//	SceneBuilder::Lights::Builder& SceneBuilder::Lights::Builder::setDirection(const glm::vec4& direction) {
//		this->lightDirection = &direction;
//		return *this;
//	}
//	SceneBuilder::Lights::Builder& SceneBuilder::Lights::Builder::setDiameter(const float& innerDiameter, const float& outerDiameter) {
//		assert(innerDiameter <= outerDiameter && "The inner diameter must be smaller than the outer diameter");
//		this->innerDiameter = &innerDiameter;
//		this->outerDiameter = &outerDiameter;
//		return *this;
//	}
//	Handle SceneBuilder::Lights::Builder::build() {
//		assert(this->position && this->type && "A light must have a position and type");
//		assert(ambient != nullptr && diffuse != nullptr && specular != nullptr && "A light must have all properties defined");
//
//		if (this->linkedHandle != nullptr) {
//			return this->sceneBuilder.addLight(*this->type, *this->position, *this->linkedHandle, *this->ambient, *this->diffuse, *this->specular, this->lightDirection, innerDiameter, outerDiameter);
//		}
//		return this->sceneBuilder.addLight(*this->type, *this->position, *this->ambient, *this->diffuse, *this->specular, this->lightDirection, innerDiameter, outerDiameter);
//	}
//
//	/* Mesh */
//
//	/* Scene Builder */
//
//	GameObject& SceneBuilder::entity(const Handle& handle) {
//		if (handle.type == Handle_Type::object) {
//			return this->objectManager.resource(handle);
//		}
//
//		throw std::runtime_error("Requested handle is not a game object handle");
//	}
//
//	Handle SceneBuilder::addObject(const std::string& pathToFile, glm::vec3& position, glm::vec3& scaleAmt,
//		Handle* materialHandle, Handle& vertShader,
//		Handle& fragShader, bool loadMaterials,
//		std::string* materialFilePath, std::string* textureDir,
//		const glm::vec4* overrideColor, const GameObjects::Builder::OverrideMaterialProperties* matPropOverride) {
//		std::unique_ptr<GameObject> object;
//		std::string materialFile = !materialFilePath ? FileHelpers::GetBaseFileDirectory(pathToFile) : *materialFilePath;
//		std::string texturePath = textureDir != nullptr ? *textureDir : FileHelpers::GetBaseFileDirectory(pathToFile);
//		Handle selectedMaterial;
//
//		if (materialHandle == nullptr) {
//			selectedMaterial = Handle::getDefault();
//		}
//		else {
//			selectedMaterial = *materialHandle;
//		}
//
//		/* Load Object From File */
//		tinyobj::attrib_t attrib;
//		std::vector<tinyobj::shape_t> shapes;
//		std::vector<tinyobj::material_t> materials;
//		std::string warn, err;
//
//		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, pathToFile.c_str(), materialFile.c_str(), true)) {
//			throw std::runtime_error(warn + err);
//		}
//		if (warn != "") {
//			std::cout << "An error occurred while loading obj file" << std::endl;
//			std::cout << warn << std::endl;
//			std::cout << "Loading will continue..." << std::endl;
//		}
//
//		float maxVal = 0;
//		size_t counter = 0;
//		size_t shapeCounter = 0;
//		size_t materialIndex = 0;
//		size_t triangleCounter = 0;
//		size_t threeCounter = 0;
//
//		Vertex* currVertex = nullptr;
//		Handle loadMaterialTexture;
//		std::unique_ptr<std::vector<Vertex>> verticies;
//		std::unique_ptr<std::vector<uint32_t>> indicies;
//		std::unique_ptr<std::vector<std::pair<unsigned int, unsigned int>>> sortedIds;
//		std::vector<std::unique_ptr<BumpMaterial>> objectMaterials;
//		std::vector<std::unique_ptr<Mesh>> meshes(shapes.size());
//		std::unique_ptr<std::vector<Triangle>> triangles;
//		tinyobj::material_t* currMaterial = nullptr;
//		std::unique_ptr<StarMaterial> objectMaterial;
//
//		if (loadMaterials && materials.size() > 0) {
//			//create needed materials
//			for (size_t i = 0; i < materials.size(); i++) {
//				currMaterial = &materials.at(i);
//				/*auto& builder = Materials::Builder(*this)
//					.setDiffuse(glm::vec4{
//						currMaterial->diffuse[0],
//						currMaterial->diffuse[1],
//						currMaterial->diffuse[2],
//						1.0f })
//						.setSpecular(glm::vec4{
//								currMaterial->specular[0],
//								currMaterial->specular[1],
//								currMaterial->specular[2],
//								1.0f })
//								.setShinyCoefficient(currMaterial->shininess);*/
//								//check if need to override texture 
//				std::unique_ptr<Texture> texture; 
//
//				if (currMaterial->diffuse_texname != "") {
//					texture = std::unique_ptr<Texture>(new Texture(texturePath + FileHelpers::GetFileNameWithExtension(currMaterial->diffuse_texname)));
//				}
//
//				//apply maps 
//				std::unique_ptr<Texture> bumpMap;
//				if (currMaterial->bump_texname != "") {
//					bumpMap = std::unique_ptr<Texture>(new Texture(texturePath + FileHelpers::GetFileNameWithExtension(currMaterial->bump_texname)));
//				}
//
//				//objectMaterialHandles.push_back(builder.build());
//				////record material to avoid multiple fetches
//				//objectMaterials.push_back(this->materialManager.resource(objectMaterialHandles.at(i)));
//				objectMaterials.push_back(std::make_unique<BumpMaterial>(glm::vec4(1.0),
//					glm::vec4(1.0),
//					glm::vec4(1.0),
//					glm::vec4{
//						currMaterial->diffuse[0],
//						currMaterial->diffuse[1],
//						currMaterial->diffuse[2],
//						1.0f },
//						glm::vec4{
//							currMaterial->specular[0],
//							currMaterial->specular[1],
//							currMaterial->specular[2],
//							1.0f },
//							currMaterial->shininess,
//							std::move(texture),
//							std::move(bumpMap)
//							));
//			}
//		}
//
//		//need to scale object so that it fits on screen
//		//combine all attributes into a single object 
//		std::array<Vertex, 3> triangleVerticies;
//		int dIndex = 0;
//		for (const auto& shape : shapes) {
//			triangleCounter = 0;
//			threeCounter = 0;
//			counter = 0;
//
//			//tinyobj ensures three verticies per triangle  -- assuming unique verticies 
//			verticies = std::make_unique<std::vector<Vertex>>(shape.mesh.indices.size());
//			const std::vector<tinyobj::index_t>& indicies = shape.mesh.indices;
//			triangles = std::make_unique<std::vector<Triangle>>(shape.mesh.material_ids.size());
//
//			for (size_t faceIndex = 0; faceIndex < shape.mesh.material_ids.size(); faceIndex++) {
//				for (int i = 0; i < 3; i++) {
//					dIndex = (3 * faceIndex) + i;
//					triangleVerticies[i].pos = {
//						attrib.vertices[3 * indicies[dIndex].vertex_index + 0],
//						attrib.vertices[3 * indicies[dIndex].vertex_index + 1],
//						attrib.vertices[3 * indicies[dIndex].vertex_index + 2],
//					};
//
//					//check for color override
//					if (!overrideColor) {
//						triangleVerticies[i].color = {
//							attrib.colors[3 * indicies[dIndex].vertex_index + 0],
//							attrib.colors[3 * indicies[dIndex].vertex_index + 1],
//							attrib.colors[3 * indicies[dIndex].vertex_index + 2],
//						};
//					}
//					else {
//						triangleVerticies[i].color = {
//							overrideColor->r,
//							overrideColor->g,
//							overrideColor->b
//						};
//					}
//
//					if (attrib.normals.size() > 0) {
//						triangleVerticies[i].normal = {
//							attrib.normals[3 * indicies[dIndex].normal_index + 0],
//							attrib.normals[3 * indicies[dIndex].normal_index + 1],
//							attrib.normals[3 * indicies[dIndex].normal_index + 2],
//						};
//					}
//
//					triangleVerticies[i].texCoord = {
//						attrib.texcoords[2 * indicies[dIndex].texcoord_index + 0],
//						1.0f - attrib.texcoords[2 * indicies[dIndex].texcoord_index + 1]
//					};
//					if (loadMaterials && shape.mesh.material_ids.at(faceIndex) != -1) {
//						//use the overridden material if provided, otherwise use the prop from mtl file
//						triangleVerticies[i].matAmbient = (matPropOverride != nullptr && matPropOverride->ambient != nullptr) ? *matPropOverride->ambient : objectMaterials.at(shape.mesh.material_ids.at(faceIndex))->ambient;
//						triangleVerticies[i].matDiffuse = (matPropOverride != nullptr && matPropOverride->diffuse != nullptr) ? *matPropOverride->diffuse : objectMaterials.at(shape.mesh.material_ids.at(faceIndex))->diffuse;
//						triangleVerticies[i].matSpecular = (matPropOverride != nullptr && matPropOverride->specular != nullptr) ? *matPropOverride->specular : objectMaterials.at(shape.mesh.material_ids.at(faceIndex))->specular;
//						triangleVerticies[i].matShininess = (matPropOverride != nullptr && matPropOverride->shiny != nullptr) ? *matPropOverride->shiny : objectMaterials.at(shape.mesh.material_ids.at(faceIndex))->shinyCoefficient;
//					}
//				}
//
//				triangles->at(faceIndex) = Triangle(triangleVerticies);
//			}
//
//			if (loadMaterials && shape.mesh.material_ids.at(shapeCounter) != -1) {
//				//apply material from files to mesh -- will ignore passed values 
//				meshes.at(shapeCounter) = std::make_unique<Mesh>(std::move(triangles), std::move(objectMaterials.at(shape.mesh.material_ids[0]))); 
//			}
//			shapeCounter++;
//		}
//
//		std::cout << "Loaded: " << pathToFile << std::endl;
//
//		//TODO: give texture handle to material 
//		return this->objectManager.addResource(std::make_unique<GameObject>(position, scaleAmt, vertShader, fragShader, std::move(meshes)));
//	}
//
//
//	Light& SceneBuilder::light(const Handle& handle) {
//		assert(handle.type == Handle_Type::light && "The requested handle is not associated with a light object");
//
//		return lightManager.resource(handle);
//	}
//
//	//Handle SceneBuilder::addMaterial(const glm::vec4& surfaceColor, const glm::vec4& hightlightColor, const glm::vec4& ambient,
//	//	const glm::vec4& diffuse, const glm::vec4& specular,
//	//	const int& shinyCoefficient, Handle* texture,
//	//	Handle* bumpMap) {
//	//	if (texture != nullptr && bumpMap != nullptr) {
//	//		return this->materialManager.add(surfaceColor, hightlightColor, ambient, diffuse, specular, shinyCoefficient, *texture, *bumpMap);
//	//	}
//	//	return this->materialManager.add(surfaceColor, hightlightColor, ambient, diffuse, specular, shinyCoefficient);
//	//}
//
//	Handle SceneBuilder::addLight(const Type::Light& type, const glm::vec3& position, const Handle& linkedHandle,
//		const glm::vec4& ambient, const glm::vec4& diffuse,
//		const glm::vec4& specular, const glm::vec4* direction,
//		const float* innerCutoff, const float* outerCutoff) {
//		GameObject& linkedObject = this->objectManager.resource(linkedHandle);
//		linkedObject.setPosition(position);
//		return this->lightManager.addResource(std::make_unique<Light>(type, position, linkedObject.getScale(), linkedHandle, linkedObject, ambient, diffuse, specular, direction, innerCutoff, outerCutoff));
//	}
//
//	Handle SceneBuilder::addLight(const Type::Light& type, const glm::vec3& position, const glm::vec4& ambient,
//		const glm::vec4& diffuse, const glm::vec4& specular,
//		const glm::vec4* direction, const float* innerCutoff,
//		const float* outerCutoff) {
//		return this->lightManager.addResource(std::make_unique<Light>(type, position, ambient, diffuse, specular, direction, innerCutoff, outerCutoff));
//	}
//}