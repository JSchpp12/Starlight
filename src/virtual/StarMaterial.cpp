#include "StarMaterial.hpp"

void star::StarMaterial::cleanupRender(StarDevice& device)
{
	if (this->isPrepared) {
		this->cleanup(device); 
		this->isPrepared = false; 
	}
}

void star::StarMaterial::prepRender(StarDevice& device)
{
	//since multiple meshes can share a material, ensure that the material has not already been prepared

	if (!this->isPrepared) {
		this->prep(device); 
		this->isPrepared = true; 
	}
}

void star::StarMaterial::finalizeDescriptors(StarDevice& device, std::vector<std::reference_wrapper<StarDescriptorSetLayout>> groupLayouts, StarDescriptorPool& groupPool,
	std::vector<std::unordered_map<int, vk::DescriptorSet>> globalSets, int numSwapChainImages)
{
	//only build descriptor sets if this object hasnt already been initialized
	if (this->descriptorSets.size() == 0) {
		for (int i = 0; i < numSwapChainImages; i++) {
			auto allDescriptors = std::vector<vk::DescriptorSet>();

			if (groupLayouts.size() >= 3) {
				auto& groupLayout = groupLayouts.at(2); 
				vk::DescriptorSet newDescriptor = this->buildDescriptorSet(device, groupLayout, groupPool, i);
				//another sign I need a better wrapper for descriptor set creation
				if (newDescriptor)
				{
					allDescriptors.resize(1 + globalSets.at(i).size());
					allDescriptors.at(2) = newDescriptor;
				}
				else {
					allDescriptors.resize(globalSets.at(i).size()); 
				}
			}else
				allDescriptors.resize(globalSets.at(i).size());

			//ignoring set number for now, should rework this system in future
			for (auto& it : globalSets.at(i)) {
				allDescriptors.at(it.first) = it.second;
			}

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

std::vector<std::pair<vk::DescriptorType, const int>> star::StarMaterial::getDescriptorRequests(const int& numFramesInFlight)
{
	return std::vector<std::pair<vk::DescriptorType, const int>>();
}

void star::StarMaterial::createDescriptors(star::StarDevice& device, const int& numFramesInFlight)
{
}

