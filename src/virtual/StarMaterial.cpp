#include "StarMaterial.hpp"

void star::StarMaterial::prepareRender(StarDevice& device)
{
	//since multiple meshes can share a material, ensure that the material has not already been prepared

	if (!this->isPrepared) {
		this->prepRender(device); 
		this->isPrepared = true; 
	}
}

void star::StarMaterial::buildDescriptorSets(StarDevice& device, StarDescriptorSetLayout& groupLayout, StarDescriptorPool& groupPool,
	std::vector<std::vector<vk::DescriptorSet>> globalSets, int numSwapChainImages)
{
	//only build descriptor sets if this object hasnt already been initialized
	if (this->descriptorSets.size() == 0) {
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
}

void star::StarMaterial::bind(vk::CommandBuffer& commandBuffer, vk::PipelineLayout pipelineLayout, int swapChainImageIndex)
{
	//bind the descriptor sets for the given image index
	auto& descriptors = this->descriptorSets.at(swapChainImageIndex); 

	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptors.size(), descriptors.data(), 0, nullptr);
}
