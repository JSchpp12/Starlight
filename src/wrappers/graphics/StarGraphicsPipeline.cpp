#include "StarGraphicsPipeline.hpp"

namespace star {
StarGraphicsPipeline::StarGraphicsPipeline(StarDevice& device, StarShader inVertShader, StarShader inFragShader, 
	PipelineConfigSettings& configSettings) 
	: StarPipeline(device), configSettings(configSettings), vertShader(inVertShader), fragShader(inFragShader) {
	this->hash = inVertShader.getPath() + inFragShader.getPath(); 
}

StarGraphicsPipeline::~StarGraphicsPipeline()
{
}

void StarGraphicsPipeline::bind(vk::CommandBuffer& commandBuffer) {
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, this->pipeline);
}

void StarGraphicsPipeline::defaultPipelineConfigInfo(PipelineConfigSettings& configSettings, vk::Extent2D swapChainExtent, vk::RenderPass renderPass, vk::PipelineLayout pipelineLayout) {
	configSettings.swapChainExtent = swapChainExtent; 
	configSettings.pipelineLayout = pipelineLayout; 
	configSettings.renderPass = renderPass; 

	vk::Rect2D scissor{};
	scissor.offset = vk::Offset2D{ 0, 0 };
	scissor.extent = configSettings.swapChainExtent;
	configSettings.scissor = scissor; 

	vk::PipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = vk::StructureType::ePipelineViewportStateCreateInfo;
	viewportState.viewportCount = 1;
	viewportState.pViewports = VK_NULL_HANDLE;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &configSettings.scissor;
	
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

	{
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
		configSettings.multisampleInfo = multisampling;
	}

	{
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
		configSettings.depthStencilInfo = depthStencil;
	}

	{
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
		//colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
		colorBlendAttachment.blendEnable = VK_FALSE;
		configSettings.colorBlendAttachment = colorBlendAttachment;
	}

	{
		vk::PipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = vk::StructureType::ePipelineColorBlendStateCreateInfo;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = vk::LogicOp::eCopy;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &configSettings.colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;
		configSettings.colorBlendInfo = colorBlending;
	}

	{
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = vk::StructureType::ePipelineInputAssemblyStateCreateInfo;
		inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
		inputAssembly.primitiveRestartEnable = VK_FALSE;
		configSettings.inputAssemblyInfo = inputAssembly; 
	}

	configSettings.rasterizationInfo = rasterizer;
	configSettings.viewportInfo = viewportState;
}

vk::Pipeline StarGraphicsPipeline::buildPipeline()
{
	vk::ShaderModule vertShaderModule = createShaderModule(*vertShader.compile());
	vk::ShaderModule fragShaderModule = createShaderModule(*fragShader.compile());

	//TODO: move this out of here
	vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
	vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main"; //the function to invoke in the shader module

	vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
	fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	auto bindingDescriptions = VulkanVertex::getBindingDescription();
	auto attributeDescriptions = VulkanVertex::getAttributeDescriptions();

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = vk::StructureType::ePipelineVertexInputStateCreateInfo;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescriptions;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	//store these creation infos for later use 
	vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	/* Dynamic State */
	//some parts of the pipeline can be changed without recreating the entire pipeline
	//if this is defined, the data for the dynamic structures will have to be provided at draw time
	vk::DynamicState dynamicStates[] = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eLineWidth
	};
	vk::PipelineDynamicStateCreateInfo dynamicStateInfo{};
	dynamicStateInfo.sType = vk::StructureType::ePipelineDynamicStateCreateInfo;
	dynamicStateInfo.dynamicStateCount = 2;
	dynamicStateInfo.pDynamicStates = dynamicStates;

	/* Pipeline */
	vk::GraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = vk::StructureType::eGraphicsPipelineCreateInfo;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;

	//ref all previously created structs
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &configSettings.inputAssemblyInfo;
	pipelineInfo.pViewportState = &configSettings.viewportInfo;
	pipelineInfo.pRasterizationState = &configSettings.rasterizationInfo;
	pipelineInfo.pMultisampleState = &configSettings.multisampleInfo;
	pipelineInfo.pDepthStencilState = &configSettings.depthStencilInfo;
	pipelineInfo.pColorBlendState = &configSettings.colorBlendInfo;
	pipelineInfo.pDynamicState = &dynamicStateInfo;
	pipelineInfo.layout = configSettings.pipelineLayout;
	//render pass info - ensure renderpass is compatible with pipeline --check khronos docs
	pipelineInfo.renderPass = configSettings.renderPass;
	pipelineInfo.subpass = 0; //index where the graphics pipeline will be used 
	//allow switching to new pipeline (inheritance) 
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional -- handle to existing pipeline that is being switched to
	pipelineInfo.basePipelineIndex = -1; // Optional

	//finally creating the pipeline -- this call has the capability of creating multiple pipelines in one call
	//2nd arg is set to null -> normally for graphics pipeline cache (can be used to store and reuse data relevant to pipeline creation across multiple calls to vkCreateGraphicsPipeline)

	auto result = this->device.getDevice().createGraphicsPipelines(VK_NULL_HANDLE, pipelineInfo);
	if (result.result != vk::Result::eSuccess) {
		throw std::runtime_error("failed to create graphics pipeline");
	}
	if (result.value.size() > 1) {
		throw std::runtime_error("unknown error has occurred, more than one pipeline was created ");
	}

	//destroy the shader modules that were created 
	this->device.getDevice().destroyShaderModule(vertShaderModule);
	this->device.getDevice().destroyShaderModule(fragShaderModule);

	return result.value.at(0);
}

}