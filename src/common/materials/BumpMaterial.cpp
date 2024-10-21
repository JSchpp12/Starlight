#include "BumpMaterial.hpp"

void star::BumpMaterial::applyDescriptorSetLayouts(star::StarDescriptorSetLayout::Builder& constBuilder)
{
	constBuilder.addBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
	constBuilder.addBinding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
}

void star::BumpMaterial::cleanup(StarDevice& device)
{
	if (this->texture){
		this->texture->cleanupRender(device);
		this->texture.reset(); 
	}

	if (this->bumpMap) {
		this->bumpMap->cleanupRender(device);
		this->bumpMap.reset(); 
	}
}

void star::BumpMaterial::buildDescriptorSet(StarDevice& device, StarShaderInfo::Builder& builder, const int& imageInFlightIndex)

{
	builder.add(*texture);
	builder.add(*bumpMap);
}

void star::BumpMaterial::prep(StarDevice& device)
{
	this->TextureMaterial::prep(device);
	bumpMap->prepRender(device);
}

std::vector<std::pair<vk::DescriptorType, const int>> star::BumpMaterial::getDescriptorRequests(const int& numFramesInFlight)
{
	return std::vector<std::pair<vk::DescriptorType, const int>>{
		std::pair<vk::DescriptorType, const int>(vk::DescriptorType::eCombinedImageSampler, 1)
	};
}
