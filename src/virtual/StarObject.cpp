#include "StarObject.hpp"

std::unique_ptr<star::StarDescriptorSetLayout> star::StarObject::instanceDescriptorLayout = std::unique_ptr<star::StarDescriptorSetLayout>(); 
vk::PipelineLayout star::StarObject::extrusionPipelineLayout = vk::PipelineLayout{}; 
std::unique_ptr<star::StarGraphicsPipeline> star::StarObject::tri_normalExtrusionPipeline = std::unique_ptr<star::StarGraphicsPipeline>(); 
std::unique_ptr<star::StarGraphicsPipeline> star::StarObject::triAdj_normalExtrusionPipeline = std::unique_ptr<star::StarGraphicsPipeline>();
std::unique_ptr<star::StarDescriptorSetLayout> star::StarObject::boundDescriptorLayout = std::unique_ptr<star::StarDescriptorSetLayout>();
vk::PipelineLayout star::StarObject::boundPipelineLayout = vk::PipelineLayout{}; 
std::unique_ptr<star::StarGraphicsPipeline> star::StarObject::boundBoxPipeline = std::unique_ptr<star::StarGraphicsPipeline>(); 

void star::StarObject::initSharedResources(StarDevice& device, vk::Extent2D swapChainExtent, 
	vk::RenderPass renderPass, int numSwapChainImages, 
	StarDescriptorSetLayout& globalDescriptors)
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
		StarGraphicsPipeline::defaultPipelineConfigInfo(settings, swapChainExtent, renderPass, extrusionPipelineLayout);
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
			instanceDescriptorLayout->getDescriptorSetLayout()
		};

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
		pipelineLayoutInfo.setLayoutCount = globalLayouts.size();
		pipelineLayoutInfo.pSetLayouts = globalLayouts.data();
		pipelineLayoutInfo.pPushConstantRanges = nullptr;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		boundPipelineLayout = device.getDevice().createPipelineLayout(pipelineLayoutInfo);

		StarGraphicsPipeline::PipelineConfigSettings settings;
		StarGraphicsPipeline::defaultPipelineConfigInfo(settings, swapChainExtent, renderPass, extrusionPipelineLayout);

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

	for (auto& iBuffer : this->instanceUniformBuffers) {
		for (auto& buf : iBuffer)
			buf.reset(); 
	}

	//cleanup any materials
	for (auto& mesh : this->getMeshes()) {
		mesh->getMaterial().cleanupRender(device); 
	}

	//delete pipeline if owns one
	if (this->pipeline)
		this->pipeline.reset(); 
}

std::unique_ptr<star::StarPipeline> star::StarObject::buildPipeline(StarDevice& device, vk::Extent2D swapChainExtent, 
	vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass)
{
	StarGraphicsPipeline::PipelineConfigSettings settings;
	StarGraphicsPipeline::defaultPipelineConfigInfo(settings, swapChainExtent, renderPass, pipelineLayout);
	auto graphicsShaders = this->getShaders();

	auto newPipeline = std::make_unique<StarGraphicsPipeline>(device, settings, graphicsShaders.at(Shader_Stage::vertex), graphicsShaders.at(Shader_Stage::fragment));
	newPipeline->init();

	return std::move(newPipeline);
}

void star::StarObject::prepRender(star::StarDevice& device, vk::Extent2D swapChainExtent,
	vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass, int numSwapChainImages, 
	std::vector<std::reference_wrapper<StarDescriptorSetLayout>> groupLayout, std::vector<std::vector<vk::DescriptorSet>> globalSets)
{
	//handle pipeline infos
	this->pipeline = this->buildPipeline(device, swapChainExtent, pipelineLayout, renderPass);

	createInstanceBuffers(device, numSwapChainImages); 
	prepareMeshes(device); 
	StarDescriptorPool& pool = ManagerDescriptorPool::getPool(); 
	prepareDescriptors(device, numSwapChainImages, groupLayout, globalSets);
}

void star::StarObject::prepRender(star::StarDevice& device, int numSwapChainImages, 
	std::vector<std::reference_wrapper<StarDescriptorSetLayout>> groupLayout,
	std::vector<std::vector<vk::DescriptorSet>> globalSets, StarPipeline& sharedPipeline)
{
	this->sharedPipeline = &sharedPipeline;

	createInstanceBuffers(device, numSwapChainImages);
	prepareMeshes(device); 
	prepareDescriptors(device, numSwapChainImages, groupLayout, globalSets);
}

void star::StarObject::recordRenderPassCommands(StarCommandBuffer& commandBuffer, vk::PipelineLayout& pipelineLayout, 
	int swapChainIndexNum, uint32_t vb_start, uint32_t ib_start) {
	RenderResourceSystem::bind(*this->indBuffer, commandBuffer.buffer(swapChainIndexNum));
	RenderResourceSystem::bind(*this->vertBuffer, commandBuffer.buffer(swapChainIndexNum));

	if (this->pipeline)
		this->pipeline->bind(commandBuffer.buffer(swapChainIndexNum)); 

	uint32_t vbStartIndex = vb_start; 
	uint32_t ibStartIndex = ib_start; 

	for (auto& rmesh : this->getMeshes()) {

		rmesh->getMaterial().bind(commandBuffer, pipelineLayout, swapChainIndexNum);

		uint32_t instanceCount = static_cast<uint32_t>(this->instances.size()); 
		uint32_t vertexCount = rmesh->getNumVerts();
		uint32_t indexCount = rmesh->getNumIndices();
		commandBuffer.buffer(swapChainIndexNum).drawIndexed(indexCount, instanceCount, ibStartIndex, 0, 0);

		vbStartIndex += rmesh->getNumVerts();
		ibStartIndex += rmesh->getNumIndices();
	}

	if (this->drawNormals)
		recordDrawCommandNormals(commandBuffer, ib_start, swapChainIndexNum); 
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
	//map needed buffers before write
	for (auto& buff : this->instanceUniformBuffers[swapChainTarget]) {
		buff->map(); 
	}

	for (int i = 0; i < this->instanceUniformBuffers[swapChainTarget].size(); i++) {
		for (auto& instance : this->instances) {
			instance->updateBufferData(*this->instanceUniformBuffers[swapChainTarget][i], i);
		}
	}
	
	//unmap when done
	for (auto& buff : this->instanceUniformBuffers[swapChainTarget]) {
		buff->unmap();
	}
}

std::vector<std::unique_ptr<star::StarDescriptorSetLayout>> star::StarObject::getDescriptorSetLayouts(StarDevice& device)
{
	auto allSets = std::vector<std::unique_ptr<star::StarDescriptorSetLayout>>(); 
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
	std::vector<std::reference_wrapper<StarDescriptorSetLayout>> fullGroupLayout, 
	std::vector<std::vector<vk::DescriptorSet>> globalSets)
{
	auto& groupLayout = fullGroupLayout.at(1);
	StarDescriptorPool& pool = ManagerDescriptorPool::getPool(); 
	std::vector<std::unordered_map<int, vk::DescriptorSet>> finalizedSets; 
	this->boundingDescriptors.resize(numSwapChainImages); 


	for (int i = 0; i < numSwapChainImages; i++) {
		std::unordered_map<int, vk::DescriptorSet> set; 
		for (int j = 0; j < globalSets.at(i).size(); j++) {
			set[j] = globalSets.at(i).at(j); 
			this->boundingDescriptors.at(i).push_back(set[j]); 
		}

		star::StarDescriptorWriter writer = star::StarDescriptorWriter(device, groupLayout, ManagerDescriptorPool::getPool());

		std::vector<vk::DescriptorBufferInfo> bufferInfos = std::vector<vk::DescriptorBufferInfo>(this->instances.front()->getBufferInfoSize().size());

		for (int j = 0; j < this->instances.front()->getBufferInfoSize().size(); j++) {
			vk::DeviceSize bufferSize = this->instances.front()->getBufferInfoSize().at(j) * this->instances.size();

			//add descriptor for uniform buffer object
			bufferInfos.at(j) = vk::DescriptorBufferInfo{
				this->instanceUniformBuffers[i][j]->getBuffer(),
				0,
				bufferSize };
			writer.writeBuffer(j, bufferInfos.at(j));
		}
		set[1] = writer.build();

		finalizedSets.push_back(set); 
		this->boundingDescriptors.at(i).push_back(set[1]);
	}
	
	
	for (auto& mesh : this->getMeshes()) {
		//descriptors
		mesh->getMaterial().finalizeDescriptors(device, fullGroupLayout, pool, finalizedSets, numSwapChainImages);
	}
}

void star::StarObject::createInstanceBuffers(star::StarDevice& device, int numImagesInFlight)
{
	assert(this->instances.size() > 0 && "Call to create instance buffers made but this object does not have any instances");
	assert(this->instances.size() < 1024 && "Max number of supported instances is 1024"); 
	
	this->instanceUniformBuffers.resize(numImagesInFlight);

	//each instance must provide the number of buffers that it will need
	//this is a bit limiting and might need some rework
	auto minProp = device.getPhysicalDevice().getProperties().limits.minUniformBufferOffsetAlignment;
	uint32_t instanceCount = static_cast<uint32_t>(this->instances.size());
	auto requestedBufferPerInstanceSize = this->instances.front()->getBufferInfoSize(); 

	//create a buffer for each image
	for (int i = 0; i < numImagesInFlight; i++) {
		for (int j = 0; j < requestedBufferPerInstanceSize.size(); j++) {
			auto perInstanceSize = requestedBufferPerInstanceSize.at(j);
			vk::DeviceSize minAlignmentOfUBOElements = StarBuffer::getAlignment(perInstanceSize, minProp);
			vk::DeviceSize size = minAlignmentOfUBOElements * instanceCount;

			this->instanceUniformBuffers.at(i).emplace_back(std::make_unique<StarBuffer>(device, size, instanceCount, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible, minProp));
		}
	}
}

void star::StarObject::recordDrawCommandNormals(star::StarCommandBuffer& commandBuffer, uint32_t ib_start, int inFlightIndex)
{
	uint32_t ib = ib_start;

	//assuming all meshes have same packing approach at this point. Should have checked earlier on during load time
	bool useAdjPipe = this->getMeshes().front()->hasAdjacentVertsPacked();

	if (useAdjPipe)
		StarObject::triAdj_normalExtrusionPipeline->bind(commandBuffer.buffer(inFlightIndex));
	else
		StarObject::tri_normalExtrusionPipeline->bind(commandBuffer.buffer(inFlightIndex)); 

	commandBuffer.buffer(inFlightIndex).setLineWidth(1.0f); 

	for (auto& rmesh : this->getMeshes()) {
		uint32_t instanceCount = static_cast<uint32_t>(this->instances.size());
		uint32_t indexCount = rmesh->getNumIndices();
		commandBuffer.buffer(inFlightIndex).drawIndexed(indexCount, instanceCount, ib, 0, 0);

		ib += rmesh->getNumIndices();
	}
}

void star::StarObject::recordDrawCommandBoundingBox(star::StarCommandBuffer& commandBuffer, int inFlightIndex)
{
	auto& buffer = commandBuffer.buffer(inFlightIndex); 

	vk::DeviceSize offsets{}; 
	buffer.bindVertexBuffers(0, this->boundingBoxVertBuffer->getBuffer(), offsets); 
	buffer.bindIndexBuffer(this->boundingBoxIndBuffer->getBuffer(), 0, vk::IndexType::eUint32); 

	this->boundBoxPipeline->bind(buffer); 
	buffer.setLineWidth(1.0f);
	//buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, extrusionPipelineLayout, 0, this->boundingDescriptors.at(inFlightIndex).size(), this->boundingDescriptors.at(inFlightIndex).data(), 0, nullptr);

	buffer.drawIndexed(this->boundingBoxIndsCount, 1, 0, 0, 0); 
}

void star::StarObject::initResources(StarDevice& device, const int numFramesInFlight)
{
	ManagerDescriptorPool::request(vk::DescriptorType::eUniformBuffer, numFramesInFlight * this->instances.size() * this->instances.front()->getBufferInfoSize().size());

	std::vector<Vertex> bbVerts; 
	std::vector<uint32_t> bbInds; 

	calculateBoundingBox(bbVerts, bbInds);
	{
		vk::DeviceSize size = sizeof(bbVerts.front());
		vk::DeviceSize bufferSize = size * bbVerts.size(); 
		uint32_t count = bbVerts.size(); 

		StarBuffer stagingBuffer{
			device,
			size,
			count,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
		}; 
		stagingBuffer.map(); 
		stagingBuffer.writeToBuffer(bbVerts.data());

		this->boundingBoxVertBuffer = std::make_unique<StarBuffer>(device,
			size,
			count,
			vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
			vk::MemoryPropertyFlagBits::eDeviceLocal);

		device.copyBuffer(stagingBuffer.getBuffer(), this->boundingBoxVertBuffer->getBuffer(), bufferSize);
	}

	{
		vk::DeviceSize size = sizeof(bbInds.front()); 
		vk::DeviceSize bufferSize = size * bbInds.size(); 
		uint32_t count = bbInds.size(); 


		StarBuffer stagingBuffer = StarBuffer(device,
			size,
			count,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostCoherent);
		stagingBuffer.map(); 
		stagingBuffer.writeToBuffer(bbInds.data()); 

		this->boundingBoxIndBuffer = std::make_unique<StarBuffer>(
			device,
			size,
			count,
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal
		);
		device.copyBuffer(stagingBuffer.getBuffer(), this->boundingBoxIndBuffer->getBuffer(), bufferSize);
	}
}

std::pair<std::unique_ptr<star::StarBuffer>, std::unique_ptr<star::StarBuffer>> star::StarObject::loadGeometryStagingBuffers(StarDevice& device, Handle& primaryVertBuffer, Handle& primaryIndexBuffer)
{
	this->vertBuffer = std::make_unique<Handle>(primaryVertBuffer);
	this->indBuffer = std::make_unique<Handle>(primaryIndexBuffer);

	return this->loadGeometryBuffers(device);
}

void star::StarObject::calculateBoundingBox(std::vector<Vertex>& verts, std::vector<uint32_t>& inds)
{
	assert(this->meshes.size() > 0 && "This function must be called after meshes are loaded");  

	glm::vec3 minBoundCoord{}, maxBoundCoord{};
	this->meshes.front()->getBoundingBoxCoords(minBoundCoord, maxBoundCoord);

	for (int i = 1; i < this->meshes.size(); i++) {
		glm::vec3 curMin{}, curMax{};
		this->meshes.at(i)->getBoundingBoxCoords(curMin, curMax);

		if (glm::all(glm::lessThan(curMin, minBoundCoord)))
			minBoundCoord = curMin; 
		
		if (glm::all(glm::greaterThan(curMax, maxBoundCoord)))
			maxBoundCoord = curMax; 
	}

	star::GeometryHelpers::calculateAxisAlignedBoundingBox(minBoundCoord, maxBoundCoord, verts, inds, true);

	this->boundingBoxIndsCount = inds.size(); 
}

void star::StarObject::destroyResources(StarDevice& device)
{
	this->boundingBoxIndBuffer.reset(); 
	this->boundingBoxVertBuffer.reset(); 
}
