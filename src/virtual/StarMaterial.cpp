#include "StarMaterial.hpp"

void star::StarMaterial::buildDescriptorSets(StarDevice& device, StarDescriptorSetLayout& groupLayout, StarDescriptorPool& groupPool, 
	std::vector<std::vector<vk::DescriptorSet>> globalSets, int numSwapChainImages)
{
	for (int i = 0; i < numSwapChainImages; i++) {
		auto allDescriptors = std::vector<vk::DescriptorSet>{
			globalSets.at(i)
		};

		vk::DescriptorSet newDescriptor = this->buildDescriptorSet(device, groupLayout, groupPool);
		
		if (newDescriptor)
			allDescriptors.push_back(newDescriptor); 

		this->descriptorSets.insert(std::pair<int, std::vector<vk::DescriptorSet>>(i, allDescriptors)); 
	}
}

void star::StarMaterial::bind(vk::CommandBuffer& commandBuffer, vk::PipelineLayout pipelineLayout, int swapChainImageIndex)
{
	//bind the descriptor sets for the given image index
	auto& descriptors = this->descriptorSets.at(swapChainImageIndex); 

	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptors.size(), descriptors.data(), 0, nullptr);
}
