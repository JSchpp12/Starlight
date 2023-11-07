#include "Square.hpp"

std::unique_ptr<star::Square> star::Square::New()
{
	return std::unique_ptr<star::Square>(new Square()); 
}

std::unique_ptr<star::StarPipeline> star::Square::buildPipeline(StarDevice& device, vk::Extent2D swapChainExtent, vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass)
{
	assert(pipelineLayout != nullptr && renderPass != nullptr && "These must be provided for now");

	StarGraphicsPipeline::PipelineConfigSettings settings;
	vk::Extent2D extent;
	star::StarGraphicsPipeline::defaultPipelineConfigInfo(settings, extent, renderPass, pipelineLayout);

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
	StarShader& vertShader = shaders.at(Shader_Stage::vertex);
	StarShader& fragShader = shaders.at(Shader_Stage::fragment);

	auto newPipeline = std::unique_ptr<StarPipeline>(new StarGraphicsPipeline(device, vertShader, fragShader, config));
	newPipeline->init();

	return std::move(newPipeline);
}

std::unordered_map<star::Shader_Stage, star::StarShader> star::Square::getShaders()
{
	std::unordered_map<star::Shader_Stage, StarShader> shaders;

	//load vertex shader
	std::string vertShaderPath = ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "/shaders/vertColor.vert";
	shaders.insert(std::pair<star::Shader_Stage, StarShader>(star::Shader_Stage::vertex, StarShader(vertShaderPath, Shader_Stage::vertex)));

	//load fragment shader
	std::string fragShaderPath = ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "/shaders/vertColor.frag";
	shaders.insert(std::pair<star::Shader_Stage, StarShader>(star::Shader_Stage::fragment, StarShader(fragShaderPath, Shader_Stage::fragment)));

	return shaders;
}

star::Square::Square()
{
	load(); 
}

void star::Square::load()
{
	std::unique_ptr<std::vector<star::Vertex>> verts = std::unique_ptr<std::vector<star::Vertex>>(new std::vector<star::Vertex>{
			star::Vertex{
				glm::vec3{-0.5f, 0.0f, -0.5f},	//position
				glm::vec3{0.0f, 1.0f, 0.0f},	//normal - posy
				glm::vec3{0.0f, 1.0f, 0.0f}		//color
			},
			star::Vertex{
				glm::vec3{0.5f, 0.0f, -0.5f},	//position
				glm::vec3{0.0f, 1.0f, 0.0f},	//normal - posy
				glm::vec3{0.0f, 1.0f, 0.0f}		//color
			},
			star::Vertex{
				glm::vec3{0.5f, 0.0f, 0.5f},	//position
				glm::vec3{0.0f, 1.0f, 0.0f},	//normal - posy
				glm::vec3{1.0f, 0.0f, 0.0f}		//color
			},
			star::Vertex{
				glm::vec3{-0.5f, 0.0f, 0.5f},	//position
				glm::vec3{0.0f, 1.0f, 0.0f},	//normal - posy
				glm::vec3{0.0f, 1.0f, 0.0f}		//color
			},
		});
	std::unique_ptr<std::vector<uint32_t>> inds = std::unique_ptr<std::vector<uint32_t>>(new std::vector<uint32_t>{
		0,3,2,0,2,1
	});

	std::unique_ptr<star::VertColorMaterial> material = std::unique_ptr<star::VertColorMaterial>(new star::VertColorMaterial());
	auto newMeshs = std::vector<std::unique_ptr<star::StarMesh>>();
	newMeshs.emplace_back(std::unique_ptr<star::StarMesh>(new star::StarMesh(std::move(verts), std::move(inds), std::move(material))));

	this->meshes = std::move(newMeshs); 
}
