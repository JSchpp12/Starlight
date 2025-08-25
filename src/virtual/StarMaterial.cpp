#include "StarMaterial.hpp"

void star::StarMaterial::cleanupRender(core::devices::DeviceContext& device)
{

}

void star::StarMaterial::prepRender(core::devices::DeviceContext& device)
{
	//since multiple meshes can share a material, ensure that the material has not already been prepared

}

void star::StarMaterial::finalizeDescriptors(core::devices::DeviceContext& device, star::StarShaderInfo::Builder builder, int numSwapChainImages)
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

bool star::StarMaterial::isKnownToBeReady(const uint8_t& frameInFlightIndex){
	return this->shaderInfo->isReady(frameInFlightIndex);
}

std::vector<std::pair<vk::DescriptorType, const int>> star::StarMaterial::getDescriptorRequests(const int& numFramesInFlight)
{
	return std::vector<std::pair<vk::DescriptorType, const int>>();
}

void star::StarMaterial::createDescriptors(star::core::devices::DeviceContext& device, const int& numFramesInFlight)
{
}

