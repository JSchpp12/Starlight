#include "StarComputePipeline.hpp"

star::StarComputePipeline::StarComputePipeline(StarDevice& device, vk::PipelineLayout& pipelineLayout, 
	StarShader inCompShader) : compShader(inCompShader), pipelineLayout(pipelineLayout), StarPipeline(device)
{
}

void star::StarComputePipeline::bind(vk::CommandBuffer& commandBuffer)
{
}

vk::Pipeline star::StarComputePipeline::buildPipeline()
{
	vk::ShaderModule compShaderModule = createShaderModule(*compShader.compile()); 

	vk::PipelineShaderStageCreateInfo compShaderStageInfo{}; 
	compShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo; 
	compShaderStageInfo.stage = vk::ShaderStageFlagBits::eCompute; 
	compShaderStageInfo.module = compShaderModule; 
	compShaderStageInfo.pName = "main"; 

	vk::PipelineLayoutCreateInfo layoutCreate; 
	return vk::Pipeline();
}