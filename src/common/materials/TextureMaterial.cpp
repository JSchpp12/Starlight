#include "TextureMaterial.hpp"

void star::TextureMaterial::prepRender(StarDevice& device)
{
	texture->prepRender(device);
}

void star::TextureMaterial::getDescriptorSetLayout(StarDescriptorSetLayout::Builder& newLayout)
{
	newLayout.addBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
}

void star::TextureMaterial::cleanupRender(StarDevice& device)
{
	if (texture)
		texture.reset(); 
}

vk::DescriptorSet star::TextureMaterial::buildDescriptorSet(StarDevice& device, StarDescriptorSetLayout& groupLayout, StarDescriptorPool& groupPool)
{
	auto sets = std::vector<vk::DescriptorSet>();
	auto writer = StarDescriptorWriter(device, groupLayout, groupPool);

	auto texInfo = vk::DescriptorImageInfo{
		texture->getSampler(),
		texture->getImageView(),
		vk::ImageLayout::eShaderReadOnlyOptimal };
	writer.writeImage(0, texInfo);

	vk::DescriptorSet newSet;
	writer.build(newSet);

	return newSet;
}
