#include "TextureMaterial.hpp"

void star::TextureMaterial::prep(StarDevice& device)
{
	texture->prepRender(device);
}

void star::TextureMaterial::initResources(StarDevice& device, const int& numFramesInFlight, const vk::Extent2D& screensize)
{

}

void star::TextureMaterial::applyDescriptorSetLayouts(star::StarDescriptorSetLayout::Builder& constBuilder)
{
	constBuilder.addBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
}

void star::TextureMaterial::cleanup(StarDevice& device)
{
	if (texture)
		texture->cleanupRender(device); 
}

void star::TextureMaterial::buildDescriptorSet(StarDevice& device, StarShaderInfo::Builder& builder, const int& imageInFlightIndex)
{
	builder.startSet();
	builder.add(this->texture->getTexture(), vk::ImageLayout::eShaderReadOnlyOptimal); 
}

std::vector<std::pair<vk::DescriptorType, const int>> star::TextureMaterial::getDescriptorRequests(const int& numFramesInFlight)
{
	return std::vector<std::pair<vk::DescriptorType, const int>>{
		std::pair<vk::DescriptorType, const int>(vk::DescriptorType::eCombinedImageSampler, 1 * numFramesInFlight)
	};
}

void star::TextureMaterial::createDescriptors(star::StarDevice& device, const int& numFramesInFlight)
{
}
