#include "StarComputePipeline.hpp"

star::StarComputePipeline::StarComputePipeline(StarDevice& device, vk::PipelineLayout& pipelineLayout, 
	StarShader inCompShader) : compShader(inCompShader), pipelineLayout(pipelineLayout), StarPipeline(device)
{
}

void star::StarComputePipeline::bind(vk::CommandBuffer& commandBuffer)
{
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, this->pipeline); 
}

vk::Pipeline star::StarComputePipeline::buildPipeline()
{
	vk::ShaderModule compShaderModule = createShaderModule(*compShader.compile()); 

	vk::PipelineShaderStageCreateInfo compShaderStageInfo{}; 
	compShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo; 
	compShaderStageInfo.stage = vk::ShaderStageFlagBits::eCompute; 
	compShaderStageInfo.module = compShaderModule; 
	compShaderStageInfo.pName = "main"; 

	vk::ComputePipelineCreateInfo createInfo{}; 
	createInfo.sType = vk::StructureType::eComputePipelineCreateInfo; 
	createInfo.layout = this->pipelineLayout; 
	createInfo.stage = compShaderStageInfo; 

	auto result = this->device.getDevice().createComputePipeline(VK_NULL_HANDLE, createInfo); 
	if (result.result != vk::Result::eSuccess) {
		throw std::runtime_error("failed to create compute pipeline"); 
	}

	this->device.getDevice().destroyShaderModule(compShaderModule); 

	return result.value; 
}