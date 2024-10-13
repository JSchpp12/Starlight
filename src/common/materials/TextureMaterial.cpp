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

vk::DescriptorSet star::TextureMaterial::buildDescriptorSet(StarDevice& device, StarDescriptorSetLayout& groupLayout, StarDescriptorPool& groupPool)
{
	auto sets = std::vector<vk::DescriptorSet>(); 
	auto layoutBuilder = star::StarDescriptorSetLayout::Builder(device); 
	auto writer = StarDescriptorWriter(device, groupLayout, groupPool);

	auto format = vk::Format::eR8G8B8A8Srgb;	
	auto texInfo = vk::DescriptorImageInfo{
		texture->getSampler(),
		texture->getImageView(),
		vk::ImageLayout::eShaderReadOnlyOptimal };
	writer.writeImage(0, texInfo);

	vk::DescriptorSet newSet = writer.build();

	return newSet;
}

std::vector<std::pair<vk::DescriptorType, const int>> star::TextureMaterial::getDescriptorRequests(const int& numFramesInFlight)
{
	return std::vector<std::pair<vk::DescriptorType, const int>>{
		std::pair<vk::DescriptorType, const int>(vk::DescriptorType::eCombinedImageSampler, 1)
	};
}

void star::TextureMaterial::createDescriptors(star::StarDevice& device, const int& numFramesInFlight)
{
}
