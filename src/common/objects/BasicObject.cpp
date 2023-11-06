#include "BasicObject.hpp"

std::unique_ptr<star::BasicObject> star::BasicObject::New(std::string objPath)
{
	return std::unique_ptr<BasicObject>(new BasicObject(objPath));
}

std::unique_ptr<star::StarPipeline> star::BasicObject::buildPipeline(StarDevice& device,
	vk::Extent2D swapChainExtent, vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass)
{
	assert(pipelineLayout != nullptr && renderPass != nullptr && "These must be provided for now"); 

	StarGraphicsPipeline::PipelineConfigSettings settings; 
	vk::Extent2D extent; 
	star::StarGraphicsPipeline::defaultPipelineConfigInfo(settings, extent); 

	/* Scissor */
	//this defines in which regions pixels will actually be stored. 
	//any pixels outside will be discarded 

	//we just want to draw the whole framebuffer for now
	vk::Rect2D scissor{};
	//scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;

	/* Viewport */
	//Viewport describes the region of the framebuffer where the output will be rendered to
	vk::Viewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;

	viewport.width = (float)swapChainExtent.width;
	viewport.height = (float)swapChainExtent.height;
	//Specify values range of depth values to use for the framebuffer. If not doing anything special, leave at default
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	//put scissor and viewport together into struct for creation 
	vk::PipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = vk::StructureType::ePipelineViewportStateCreateInfo;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	/* Rasterizer */
		//takes the geometry and creates fragments which are then passed onto the fragment shader 
		//also does: depth testing, face culling, and the scissor test
	vk::PipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = vk::StructureType::ePipelineRasterizationStateCreateInfo;
	//if set to true -> fragments that are beyond near and far planes are set to those distances rather than being removed
	rasterizer.depthClampEnable = VK_FALSE;

	//polygonMode determines how frags are generated. Different options: 
	//1. VK_POLYGON_MODE_FILL: fill the area of the polygon with fragments
	//2. VK_POLYGON_MODE_LINE: polygon edges are drawn as lines 
	//3. VK_POLYGON_MODE_POINT: polygon verticies are drawn as points
	//NOTE: using any other than fill, requires GPU feature
	rasterizer.polygonMode = vk::PolygonMode::eFill;

	//available line widths, depend on GPU. If above 1.0f, required wideLines GPU feature
	rasterizer.lineWidth = 1.0f; //measured in fragment widths

	//cullMode : type of face culling to use.
	rasterizer.cullMode = vk::CullModeFlagBits::eBack;
	rasterizer.frontFace = vk::FrontFace::eCounterClockwise;

	//depth values can be used in way that is known as 'shadow mapping'. 
	//rasterizer is capable of changing depth values through constant addition or biasing based on frags slope 
	//this is left as off for now 
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; //optional 
	rasterizer.depthBiasClamp = 0.0f; //optional 
	rasterizer.depthBiasSlopeFactor = 0.0f; //optional

	/* Multisampling */
	//this is one of the methods of performing anti-aliasing
	//enabling requires GPU feature -- left off for this tutorial 
	vk::PipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = vk::StructureType::ePipelineMultisampleStateCreateInfo;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
	multisampling.minSampleShading = 1.0f; //optional 
	multisampling.pSampleMask = nullptr; //optional
	multisampling.alphaToCoverageEnable = VK_FALSE; //optional
	multisampling.alphaToOneEnable = VK_FALSE; //optional

	/* Depth and Stencil Testing */
	//if using depth or stencil buffer, a depth and stencil tests are neeeded
	vk::PipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = vk::StructureType::ePipelineDepthStencilStateCreateInfo;
	depthStencil.depthTestEnable = VK_TRUE;             //specifies if depth of new fragments should be compared to the depth buffer to test for actual display state
	depthStencil.depthWriteEnable = VK_TRUE;            //specifies if the new depth of fragments that pass the depth tests should be written to the depth buffer 
	depthStencil.depthCompareOp = vk::CompareOp::eLess;   //comparison that is performed to keep or discard fragments - here this is: lower depth = closer, so depth of new frags should be less
	//following are for optional depth bound testing - keep frags within a specific range 
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;                 //optional 
	depthStencil.maxDepthBounds = 1.0f;                 //optional
	//following are used for stencil tests - make sure that format of depth image contains a stencil component
	depthStencil.stencilTestEnable = VK_FALSE;

	/* Color blending */
	// after the fragShader has returned a color, it must be combined with the color already in the framebuffer
	// there are two ways to do this: 
	//      1. mix the old and new value to produce final color
	//      2. combine the old a new value using a bitwise operation 
	//two structs are needed to create this functionality: 
	//  1. VkPipelineColorBlendAttachmentState: configuration per attached framebuffer 
	//  2. VkPipelineColorBlendStateCreateInfo: global configuration
	//only using one framebuffer in this project -- both of these are disabled in this project
	vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	colorBlendAttachment.blendEnable = VK_FALSE;

	vk::PipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = vk::StructureType::ePipelineColorBlendStateCreateInfo;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = vk::LogicOp::eCopy;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	StarGraphicsPipeline::PipelineConfigSettings config{};
	config.viewportInfo = viewportState;
	config.rasterizationInfo = rasterizer;
	config.multisampleInfo = multisampling;
	config.depthStencilInfo = depthStencil;
	config.colorBlendInfo = colorBlending;
	config.colorBlendAttachment = colorBlendAttachment;
	config.pipelineLayout = pipelineLayout;
	config.renderPass = renderPass;

	auto shaders = this->getShaders(); 
	StarShader vertShader = shaders.at(Shader_Stage::vertex); 
	StarShader fragShader = shaders.at(Shader_Stage::fragment); 

	auto newPipeline = std::unique_ptr<StarPipeline>(new StarGraphicsPipeline(device, vertShader, fragShader, config));
	newPipeline->init(swapChainExtent); 
	
	return std::move(newPipeline); 
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
	std::vector<std::shared_ptr<StarMaterial>> preparedMaterials; 


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

			//check if any material values are 0 - ambient is important
			if (currMaterial->ambient[0] == 0) {
				currMaterial->ambient[0] = 1.0; 
				currMaterial->ambient[1] = 1.0; 
				currMaterial->ambient[2] = 1.0; 
			}

			if (bumpMap)
			{
				this->isBumpyMaterial = true; 
				preparedMaterials.push_back(std::shared_ptr<BumpMaterial>(new BumpMaterial(glm::vec4(1.0),
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
			else {
				preparedMaterials.push_back(std::shared_ptr<TextureMaterial>(new TextureMaterial(glm::vec4(1.0),
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
							std::move(texture)
							)));
			}

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

					vertices->at(vertCounter) = newVertex;
					fullInd->at(vertCounter) = star::CastHelpers::size_t_to_unsigned_int(vertCounter); 
					vertCounter++; 
				};
			}

			if (shape.mesh.material_ids.at(shapeCounter) != -1) {
				//apply material from files to mesh -- will ignore passed values 
				meshes.at(shapeCounter) = std::unique_ptr<StarMesh>(new StarMesh(std::move(vertices), std::move(fullInd), preparedMaterials.at(shape.mesh.material_ids[0])));
			}
			shapeCounter++;
		}
	}

	this->meshes = std::move(meshes); 
}

std::unordered_map<star::Shader_Stage, star::StarShader> star::BasicObject::getShaders()
{
	std::unordered_map<star::Shader_Stage, StarShader> shaders; 
	
	if (isBumpyMaterial) {
		//load vertex shader
		std::string vertShaderPath = ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "/shaders/bump.vert";
		shaders.insert(std::pair<star::Shader_Stage, StarShader>(star::Shader_Stage::vertex, StarShader(vertShaderPath, Shader_Stage::vertex)));

		//load fragment shader
		std::string fragShaderPath = ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "/shaders/bump.frag";
		shaders.insert(std::pair<star::Shader_Stage, StarShader>(star::Shader_Stage::fragment, StarShader(fragShaderPath, Shader_Stage::fragment)));
	}
	else {
		//load vertex shader
		std::string vertShaderPath = ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "/shaders/default.vert";
		shaders.insert(std::pair<star::Shader_Stage, StarShader>(star::Shader_Stage::vertex, StarShader(vertShaderPath, Shader_Stage::vertex)));

		//load fragment shader
		std::string fragShaderPath = ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "/shaders/default.frag";
		shaders.insert(std::pair<star::Shader_Stage, StarShader>(star::Shader_Stage::fragment, StarShader(fragShaderPath, Shader_Stage::fragment)));
	}
			
	return shaders; 
}
