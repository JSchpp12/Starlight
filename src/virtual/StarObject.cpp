#include "StarObject.hpp"

std::unique_ptr<star::StarDescriptorSetLayout> star::StarObject::instanceDescriptorLayout = std::unique_ptr<star::StarDescriptorSetLayout>(); 
vk::PipelineLayout star::StarObject::extrusionPipelineLayout = vk::PipelineLayout{}; 
std::unique_ptr<star::StarGraphicsPipeline> star::StarObject::tri_normalExtrusionPipeline = std::unique_ptr<star::StarGraphicsPipeline>(); 
std::unique_ptr<star::StarGraphicsPipeline> star::StarObject::triAdj_normalExtrusionPipeline = std::unique_ptr<star::StarGraphicsPipeline>();
std::unique_ptr<star::StarDescriptorSetLayout> star::StarObject::boundDescriptorLayout = std::unique_ptr<star::StarDescriptorSetLayout>();
vk::PipelineLayout star::StarObject::boundPipelineLayout = vk::PipelineLayout{}; 
std::unique_ptr<star::StarGraphicsPipeline> star::StarObject::boundBoxPipeline = std::unique_ptr<star::StarGraphicsPipeline>(); 

star::StarObject::StarObject(){

}

void star::StarObject::initSharedResources(StarDevice& device, vk::Extent2D swapChainExtent, int numSwapChainImages, 
	StarDescriptorSetLayout& globalDescriptors, RenderingTargetInfo renderingInfo)
{
	std::string mediaPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory);
		
	instanceDescriptorLayout = StarDescriptorSetLayout::Builder(device)
		.addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex)
		.addBinding(1, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex)
		.build();

	//this must match the descriptors for the StarObjectInstance
	{
		std::vector<vk::DescriptorSetLayout> globalLayouts{
			globalDescriptors.getDescriptorSetLayout(),
			instanceDescriptorLayout->getDescriptorSetLayout()
		}; 

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{}; 
		pipelineLayoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo; 
		pipelineLayoutInfo.setLayoutCount = globalLayouts.size(); 
		pipelineLayoutInfo.pSetLayouts = globalLayouts.data(); 
		pipelineLayoutInfo.pPushConstantRanges = nullptr; 
		pipelineLayoutInfo.pushConstantRangeCount = 0; 
		extrusionPipelineLayout = device.getDevice().createPipelineLayout(pipelineLayoutInfo); 
	}
	{
		std::string vertPath = mediaPath + "/shaders/extrudeNormals/extrudeNormals.vert";
		std::string fragPath = mediaPath + "/shaders/extrudeNormals/extrudeNormals.frag";

		StarShader vert = StarShader(vertPath, Shader_Stage::vertex);
		StarShader frag = StarShader(fragPath, Shader_Stage::fragment);

		StarGraphicsPipeline::PipelineConfigSettings settings;
		StarGraphicsPipeline::defaultPipelineConfigInfo(settings, swapChainExtent, extrusionPipelineLayout, renderingInfo);
		{
			std::string geomPath = mediaPath + "shaders/extrudeNormals/extrudeNormals_triangle.geom";
			StarShader geom = StarShader(geomPath, Shader_Stage::geometry);

			StarObject::tri_normalExtrusionPipeline = std::make_unique<StarGraphicsPipeline>(device, settings, vert, frag, geom);
			StarObject::tri_normalExtrusionPipeline->init();
		}
		{
			settings.inputAssemblyInfo.topology = vk::PrimitiveTopology::eTriangleListWithAdjacency;

			std::string geomPath = mediaPath + "shaders/extrudeNormals/extrudeNormals_triangleAdj.geom";
			StarShader geom = StarShader(geomPath, Shader_Stage::geometry);

			StarObject::triAdj_normalExtrusionPipeline = std::make_unique<StarGraphicsPipeline>(device, settings, vert, frag, geom);
			StarObject::triAdj_normalExtrusionPipeline->init();
		}
	}

	boundDescriptorLayout = StarDescriptorSetLayout::Builder(device)
		.addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex)
		.build();

	{
		std::vector<vk::DescriptorSetLayout> globalLayouts{
			globalDescriptors.getDescriptorSetLayout(),
			boundDescriptorLayout->getDescriptorSetLayout()
		};

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
		pipelineLayoutInfo.setLayoutCount = globalLayouts.size();
		pipelineLayoutInfo.pSetLayouts = globalLayouts.data();
		pipelineLayoutInfo.pPushConstantRanges = nullptr;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		boundPipelineLayout = device.getDevice().createPipelineLayout(pipelineLayoutInfo);

		StarGraphicsPipeline::PipelineConfigSettings settings;
		StarGraphicsPipeline::defaultPipelineConfigInfo(settings, swapChainExtent, extrusionPipelineLayout, renderingInfo);

		settings.inputAssemblyInfo.topology = vk::PrimitiveTopology::eLineList;

		//bounding box vert buffer will be bound to the 1st index 
		std::string boundVertPath = mediaPath + "shaders/boundingBox/bounding.vert"; 
		std::string boundFragPath = mediaPath + "shaders/boundingBox/bounding.frag"; 
		StarShader vert = StarShader(boundVertPath, Shader_Stage::vertex);
		StarShader frag = StarShader(boundFragPath, Shader_Stage::fragment); 

		StarObject::boundBoxPipeline = std::make_unique<StarGraphicsPipeline>(device, settings, vert, frag);
		boundBoxPipeline->init(); 
	}
}

void star::StarObject::cleanupSharedResources(StarDevice& device)
{
	instanceDescriptorLayout.reset(); 
	device.getDevice().destroyPipelineLayout(extrusionPipelineLayout);
	StarObject::triAdj_normalExtrusionPipeline.reset(); 
	StarObject::tri_normalExtrusionPipeline.reset(); 

	boundDescriptorLayout.reset(); 
	device.getDevice().destroyPipelineLayout(boundPipelineLayout); 
	StarObject::boundBoxPipeline.reset(); 
}

void star::StarObject::cleanupRender(StarDevice& device)
{
	this->normalExtrusionPipeline.reset(); 

	this->setLayout.reset(); 

	//cleanup any materials
	for (auto& mesh : this->getMeshes()) {
		mesh->getMaterial().cleanupRender(device); 
	}

	//delete pipeline if owns one
	if (this->pipeline)
		this->pipeline.reset(); 
}

std::unique_ptr<star::StarPipeline> star::StarObject::buildPipeline(StarDevice& device, vk::Extent2D swapChainExtent, 
	vk::PipelineLayout pipelineLayout, RenderingTargetInfo renderInfo)
{
	StarGraphicsPipeline::PipelineConfigSettings settings;
	StarGraphicsPipeline::defaultPipelineConfigInfo(settings, swapChainExtent, pipelineLayout, renderInfo);
	auto graphicsShaders = this->getShaders();

	auto newPipeline = std::make_unique<StarGraphicsPipeline>(device, settings, graphicsShaders.at(Shader_Stage::vertex), graphicsShaders.at(Shader_Stage::fragment));
	newPipeline->init();

	return std::move(newPipeline);
}

void star::StarObject::prepRender(star::StarDevice& device, vk::Extent2D swapChainExtent,
	vk::PipelineLayout pipelineLayout, RenderingTargetInfo renderInfo, int numSwapChainImages, 
	star::StarShaderInfo::Builder fullEngineBuilder)
{
	std::vector<Vertex> bbVerts;
	std::vector<uint32_t> bbInds;

	calculateBoundingBox(bbVerts, bbInds);

	this->boundingBoxVertBuffer = ManagerRenderResource::addRequest(std::make_unique<ObjVertInfo>(bbVerts));
	this->boundingBoxIndexBuffer = ManagerRenderResource::addRequest(std::make_unique<ObjIndicesInfo>(bbInds));

	this->engineBuilder = std::make_unique<StarShaderInfo::Builder>(fullEngineBuilder);

	//handle pipeline infos
	this->pipeline = this->buildPipeline(device, swapChainExtent, pipelineLayout, renderInfo);

	createInstanceBuffers(device, numSwapChainImages); 
	prepareMeshes(device); 
}

void star::StarObject::prepRender(star::StarDevice& device, int numSwapChainImages, StarPipeline& sharedPipeline, star::StarShaderInfo::Builder fullEngineBuilder)
{
	std::vector<Vertex> bbVerts;
	std::vector<uint32_t> bbInds;

	calculateBoundingBox(bbVerts, bbInds);

	this->boundingBoxVertBuffer = ManagerRenderResource::addRequest(std::make_unique<ObjVertInfo>(bbVerts));
	this->boundingBoxIndexBuffer = ManagerRenderResource::addRequest(std::make_unique<ObjIndicesInfo>(bbInds));

	this->engineBuilder = std::make_unique<StarShaderInfo::Builder>(fullEngineBuilder); 

	this->sharedPipeline = &sharedPipeline;

	createInstanceBuffers(device, numSwapChainImages);
	prepareMeshes(device); 
}

void star::StarObject::recordRenderPassCommands(vk::CommandBuffer& commandBuffer, vk::PipelineLayout& pipelineLayout,
	int swapChainIndexNum) {
	for (auto& rmesh : this->getMeshes()){
		if (!rmesh.get()->isKnownToBeReady(swapChainIndexNum)){
			return;
		}	
	}

	if (this->pipeline)
		this->pipeline->bind(commandBuffer); 

	for (auto& rmesh : this->getMeshes()) {
		uint32_t instanceCount = static_cast<uint32_t>(this->instances.size()); 
		rmesh.get()->recordRenderPassCommands(commandBuffer, pipelineLayout, swapChainIndexNum, instanceCount);
	}

	if (this->drawNormals)
		recordDrawCommandNormals(commandBuffer); 
	if (this->drawBoundingBox)
		recordDrawCommandBoundingBox(commandBuffer, swapChainIndexNum); 
}

star::StarObjectInstance& star::StarObject::createInstance()
{
	int instanceCount = static_cast<int>(this->instances.size()); 

	this->instances.push_back(std::make_unique<StarObjectInstance>(instanceCount));
	return *this->instances.back(); 
}

void star::StarObject::prepDraw(int swapChainTarget)
{

}

std::vector<std::shared_ptr<star::StarDescriptorSetLayout>> star::StarObject::getDescriptorSetLayouts(StarDevice& device)
{
	auto allSets = std::vector<std::shared_ptr<star::StarDescriptorSetLayout>>(); 
	auto staticSetBuilder = StarDescriptorSetLayout::Builder(device);

	this->getMeshes().front()->getMaterial().applyDescriptorSetLayouts(staticSetBuilder); 

	StarDescriptorSetLayout::Builder updateSetBuilder = StarDescriptorSetLayout::Builder(device)
		.addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex)
		.addBinding(1, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);
	allSets.push_back(std::move(updateSetBuilder.build())); 

	auto staticSet = staticSetBuilder.build(); 

	if (staticSet->getBindings().size() > 0)
		allSets.push_back(std::move(staticSet)); 

	return std::move(allSets); 
}

void star::StarObject::prepareMeshes(star::StarDevice& device)
{
	for (auto& mesh : this->getMeshes()) {
		mesh->prepRender(device);
	}
}

void star::StarObject::prepareDescriptors(star::StarDevice& device, int numSwapChainImages,
	star::StarShaderInfo::Builder frameBuilder)
{
	for (int i = 0; i < numSwapChainImages; i++) {
		frameBuilder.startOnFrameIndex(i);
		frameBuilder.startSet(); 
		frameBuilder.add(this->instanceModelInfos[i]);
		frameBuilder.add(this->instanceNormalInfos[i]); 
	}
	
	for (auto& mesh : this->getMeshes()) {
		//descriptors
		mesh->getMaterial().finalizeDescriptors(device, frameBuilder, numSwapChainImages);
	}
}

void star::StarObject::createInstanceBuffers(star::StarDevice& device, int numImagesInFlight)
{
	assert(this->instances.size() > 0 && "Call to create instance buffers made but this object does not have any instances");
	assert(this->instances.size() < 1024 && "Max number of supported instances is 1024"); 

	//each instance must provide the number of buffers that it will need
	//this is a bit limiting and might need some rework

	uint32_t instanceCount = static_cast<uint32_t>(this->instances.size());

	//create a buffer for each image
	for (int i = 0; i < numImagesInFlight; i++) {
		this->instanceModelInfos.emplace_back(
			ManagerRenderResource::addRequest(std::make_unique<InstanceModelInfo>(this->instances, i))
		);
		this->instanceNormalInfos.emplace_back(
			ManagerRenderResource::addRequest(std::make_unique<InstanceNormalInfo>(this->instances, i))
		); 
	}
}

void star::StarObject::createBoundingBox(std::vector<Vertex>& verts, std::vector<uint32_t>& inds)
{
	std::array<glm::vec3, 2> bbBounds = this->meshes.front()->getBoundingBoxCoords();

	for (int i = 1; i < this->meshes.size(); i++) {
		std::array<glm::vec3, 2> curbbBounds = this->meshes.at(i)->getBoundingBoxCoords();

		if (curbbBounds[0].x < bbBounds[0].x)
			bbBounds[0].x = curbbBounds[0].x;
		if (curbbBounds[0].y < bbBounds[0].y)
			bbBounds[0].y = curbbBounds[0].y;
		if (curbbBounds[0].z < curbbBounds[0].z)
			bbBounds[0].z = curbbBounds[0].z;

		if (curbbBounds[1].x > bbBounds[1].x)
			bbBounds[1].x = curbbBounds[1].x;
		if (curbbBounds[1].y > bbBounds[1].y)
			bbBounds[1].y = curbbBounds[1].y;
		if (curbbBounds[1].z > bbBounds[1].z)
			bbBounds[1].z = curbbBounds[1].z;
	}

	star::GeometryHelpers::calculateAxisAlignedBoundingBox(bbBounds[0], bbBounds[1], verts, inds, true);
}

void star::StarObject::recordDrawCommandNormals(vk::CommandBuffer& commandBuffer)
{
	uint32_t ib = 0;

	//assuming all meshes have same packing approach at this point. Should have checked earlier on during load time
	bool useAdjPipe = this->getMeshes().front()->hasAdjacentVertsPacked();

	if (useAdjPipe)
		StarObject::triAdj_normalExtrusionPipeline->bind(commandBuffer);
	else
		StarObject::tri_normalExtrusionPipeline->bind(commandBuffer); 

	commandBuffer.setLineWidth(1.0f); 

	for (auto& rmesh : this->getMeshes()) {
		uint32_t instanceCount = static_cast<uint32_t>(this->instances.size());
		uint32_t indexCount = rmesh->getNumIndices();
		commandBuffer.drawIndexed(indexCount, instanceCount, ib, 0, 0);

		ib += rmesh->getNumIndices();
	}
}

void star::StarObject::recordDrawCommandBoundingBox(vk::CommandBuffer& commandBuffer, int inFlightIndex)
{
	vk::DeviceSize offsets{}; 
	commandBuffer.bindVertexBuffers(0, ManagerRenderResource::getBuffer(this->boundingBoxVertBuffer).getVulkanBuffer(), offsets); 
	commandBuffer.bindIndexBuffer(ManagerRenderResource::getBuffer(this->boundingBoxIndexBuffer).getVulkanBuffer(), 0, vk::IndexType::eUint32);

	this->boundBoxPipeline->bind(commandBuffer); 
	commandBuffer.setLineWidth(1.0f);

	commandBuffer.drawIndexed(this->boundingBoxIndsCount, 1, 0, 0, 0);
}

void star::StarObject::calculateBoundingBox(std::vector<Vertex>& verts, std::vector<uint32_t>& inds)
{
	assert(this->meshes.size() > 0 && "This function must be called after meshes are loaded");

	this->createBoundingBox(verts, inds);	
	this->boundingBoxIndsCount = inds.size();
}

std::vector<std::pair<vk::DescriptorType, const int>> star::StarObject::getDescriptorRequests(const int& numFramesInFlight)
{
	return std::vector<std::pair<vk::DescriptorType, const int>>{std::make_pair(vk::DescriptorType::eUniformBuffer, 2)};
}

void star::StarObject::createDescriptors(star::StarDevice& device, const int& numFramesInFlight)
{
	this->prepareDescriptors(device, numFramesInFlight, *this->engineBuilder);

	this->engineBuilder.reset(); 
}
