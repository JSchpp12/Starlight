#include "StarPipeline.hpp"

star::StarPipeline::~StarPipeline()
{
	device.getDevice().getVulkanDevice().destroyPipeline(pipeline); 
}

void star::StarPipeline::init()
{
	pipeline = this->buildPipeline();
}

bool star::StarPipeline::isSame(StarPipeline& compPipe)
{
	return (this->hash == compPipe.getHash()); 
}

vk::ShaderModule star::StarPipeline::createShaderModule(const std::vector<uint32_t>& sourceCode) {
	vk::ShaderModuleCreateInfo createInfo{};
	createInfo.sType = vk::StructureType::eShaderModuleCreateInfo;
	createInfo.codeSize = 4 * sourceCode.size();
	createInfo.pCode = sourceCode.data();

	VkShaderModule shaderModule = this->device.getDevice().getVulkanDevice().createShaderModule(createInfo);
	if (!shaderModule) {
		throw std::runtime_error("failed to create shader module");
	}
	return shaderModule;
}