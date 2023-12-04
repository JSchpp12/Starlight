#include "TextureMaterial.hpp"

void star::TextureMaterial::prep(StarDevice& device)
{
	texture->prepRender(device);
}

void star::TextureMaterial::applyDescriptorSetLayouts(star::StarDescriptorSetLayout::Builder& constBuilder, StarDescriptorSetLayout::Builder& perDrawBuilder)
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
