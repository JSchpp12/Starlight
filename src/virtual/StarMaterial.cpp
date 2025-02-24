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

void star::StarMaterial::finalizeDescriptors(StarDevice& device, star::StarShaderInfo::Builder builder, int numSwapChainImages)
{
	//only build descriptor sets if this object hasnt already been initialized
	for (int i = 0; i < numSwapChainImages; i++) {
		builder.startOnFrameIndex(i); 
		this->buildDescriptorSet(device, builder, i); 
	}

	this->shaderInfo = builder.build(); 

}

void star::StarMaterial::bind(vk::CommandBuffer& commandBuffer, vk::PipelineLayout pipelineLayout, int swapChainImageIndex)
{
	//bind the descriptor sets for the given image index
	auto descriptors = this->shaderInfo->getDescriptors(swapChainImageIndex); 
	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptors.size(), descriptors.data(), 0, nullptr);
}

bool star::StarMaterial::isReady(const uint8_t& frameInFlightIndex){
	return this->shaderInfo->isReady(frameInFlightIndex);
}

std::vector<std::pair<vk::DescriptorType, const int>> star::StarMaterial::getDescriptorRequests(const int& numFramesInFlight)
{
	return std::vector<std::pair<vk::DescriptorType, const int>>();
}

void star::StarMaterial::createDescriptors(star::StarDevice& device, const int& numFramesInFlight)
{
}

