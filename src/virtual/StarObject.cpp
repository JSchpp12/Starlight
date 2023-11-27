#include "StarObject.hpp"

void star::StarObject::cleanupRender(StarDevice& device)
{
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

	auto newPipeline = std::make_unique<StarGraphicsPipeline>(device, graphicsShaders.at(Shader_Stage::vertex), graphicsShaders.at(Shader_Stage::fragment), settings);
	newPipeline->init();

	return std::move(newPipeline);
}

void star::StarObject::initRender(int numFramesInFlight)
{
	ManagerDescriptorPool::request(vk::DescriptorType::eUniformBuffer, numFramesInFlight);
}

void star::StarObject::prepRender(star::StarDevice& device, vk::Extent2D swapChainExtent,
	vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass, int numSwapChainImages, 
	std::vector<std::unique_ptr<StarDescriptorSetLayout>>& groupLayout, std::vector<std::vector<vk::DescriptorSet>> globalSets)
{
	//handle pipeline infos
	this->pipeline = this->buildPipeline(device, swapChainExtent, pipelineLayout, renderPass);

	createInstanceBuffers(device, numSwapChainImages); 

	prepareMeshes(device); 
	StarDescriptorPool& pool = ManagerDescriptorPool::getPool(); 
	prepareDescriptors(device, numSwapChainImages, groupLayout, globalSets);
}

void star::StarObject::prepRender(star::StarDevice& device, int numSwapChainImages, 
	std::vector<std::unique_ptr<StarDescriptorSetLayout>>& groupLayout,
	std::vector<std::vector<vk::DescriptorSet>> globalSets, StarPipeline& sharedPipeline)
{
	this->sharedPipeline = &sharedPipeline;

	createInstanceBuffers(device, numSwapChainImages);
	prepareMeshes(device); 
	prepareDescriptors(device, numSwapChainImages, groupLayout, globalSets);
}

void star::StarObject::recordRenderPassCommands(StarCommandBuffer& commandBuffer, vk::PipelineLayout& pipelineLayout, 
	int swapChainIndexNum, uint32_t vb_start, uint32_t ib_start) {
	if (this->pipeline)
		this->pipeline->bind(commandBuffer.buffer(swapChainIndexNum)); 

	for (auto& rmesh : this->getMeshes()) {
		rmesh->getMaterial().bind(commandBuffer, pipelineLayout, swapChainIndexNum);

		uint32_t instanceCount = static_cast<uint32_t>(this->instances.size()); 
		uint32_t vertexCount = CastHelpers::size_t_to_unsigned_int(rmesh->getVertices().size());
		uint32_t indexCount = CastHelpers::size_t_to_unsigned_int(rmesh->getIndices().size());
		commandBuffer.buffer(swapChainIndexNum).drawIndexed(indexCount, instanceCount, ib_start, 0, 0);

		vb_start += rmesh->getVertices().size(); 
		ib_start += rmesh->getIndices().size(); 
	}
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

	StarDescriptorSetLayout::Builder updateSetBuilder = StarDescriptorSetLayout::Builder(device)
		.addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex)
		.addBinding(1, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);

	this->getMeshes().front()->getMaterial().applyDescriptorSetLayouts(staticSetBuilder, updateSetBuilder); 

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
	std::vector<std::unique_ptr<StarDescriptorSetLayout>>& fullGroupLayout, std::vector<std::vector<vk::DescriptorSet>> globalSets)
{
	auto& groupLayout = *fullGroupLayout.at(1);
	StarDescriptorPool& pool = ManagerDescriptorPool::getPool(); 

	//create descriptor layout
	this->setLayout = StarDescriptorSetLayout::Builder(device )
		.addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex)
		.addBinding(1, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex)
		.build();

	for (int i = 0; i < numSwapChainImages; i++) {
		star::StarDescriptorWriter writer = star::StarDescriptorWriter(device, *this->setLayout, ManagerDescriptorPool::getPool());

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
		vk::DescriptorSet set = writer.build(); 

		globalSets.at(i).push_back(set);
	}
	
	for (auto& mesh : this->getMeshes()) {
		//descriptors
		mesh->getMaterial().buildDescriptorSets(device, groupLayout, pool, globalSets, numSwapChainImages);
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
