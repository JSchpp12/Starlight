#include "StarObject.hpp"

void star::StarObject::cleanupRender(StarDevice& device)
{
	//cleanup any materials
	for (auto& mesh : this->getMeshes()) {
		mesh->getMaterial().cleanupRender(device); 
	}

	//delete pipeline if owns one
	if (this->pipeline)
		this->pipeline.reset(); 
}

std::unique_ptr<star::StarPipeline> star::StarObject::buildPipeline(StarDevice& device, vk::Extent2D swapChainExtent, vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass)
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
	vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass, int numSwapChainImages, StarDescriptorSetLayout& groupLayout, 
	StarDescriptorPool& groupPool, std::vector<std::vector<vk::DescriptorSet>> globalSets)
{
	//handle pipeline infos
	this->pipeline = this->buildPipeline(device, swapChainExtent, pipelineLayout, renderPass);

	prepareMeshes(device); 
	prepareDescriptors(device, numSwapChainImages, groupLayout, groupPool, globalSets); 
}

void star::StarObject::prepRender(star::StarDevice& device, int numSwapChainImages, 
	StarDescriptorSetLayout& groupLayout, StarDescriptorPool& groupPool, 
	std::vector<std::vector<vk::DescriptorSet>> globalSets, StarPipeline& sharedPipeline)
{
	this->sharedPipeline = &sharedPipeline; 

	prepareMeshes(device); 
	prepareDescriptors(device, numSwapChainImages, groupLayout, groupPool, globalSets); 
}

void star::StarObject::recordRenderPassCommands(StarCommandBuffer& commandBuffer, vk::PipelineLayout& pipelineLayout, int swapChainIndexNum, uint32_t vb_start, uint32_t ib_start) {
	if (this->pipeline)
		this->pipeline->bind(commandBuffer.buffer(swapChainIndexNum)); 

	for (auto& rmesh : this->getMeshes()) {
		rmesh->recordCommands(commandBuffer, pipelineLayout, swapChainIndexNum, vb_start, ib_start);

		vb_start += rmesh->getVertices().size(); 
		ib_start += rmesh->getIndices().size(); 
	}
}

void star::StarObject::prepareMeshes(star::StarDevice& device)
{
	for (auto& mesh : this->getMeshes()) {
		mesh->prepRender(device);
	}
}

void star::StarObject::prepareDescriptors(star::StarDevice& device, int numSwapChainImages,
	StarDescriptorSetLayout& groupLayout, StarDescriptorPool& groupPool, std::vector<std::vector<vk::DescriptorSet>> globalSets)
{
	for (auto& mesh : this->getMeshes()) {
		//descriptors
		mesh->getMaterial().buildDescriptorSets(device, groupLayout, groupPool, globalSets, numSwapChainImages);
	}
}
