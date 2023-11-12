#include "BumpMaterial.hpp"

void star::BumpMaterial::getDescriptorSetLayout(star::StarDescriptorSetLayout::Builder& newLayout)
{
	newLayout.addBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
	newLayout.addBinding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
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

vk::DescriptorSet star::BumpMaterial::buildDescriptorSet(StarDevice& device, StarDescriptorSetLayout& groupLayout,
	StarDescriptorPool& groupPool)
{
	auto sets = std::vector<vk::DescriptorSet>(); 
	auto writer = StarDescriptorWriter(device, groupLayout, groupPool); 

	auto texInfo = vk::DescriptorImageInfo{
		texture->getSampler(),
		texture->getImageView(),
		vk::ImageLayout::eShaderReadOnlyOptimal };
	writer.writeImage(0, texInfo);

	auto bumpInfo = vk::DescriptorImageInfo{
		bumpMap->getSampler(),
		bumpMap->getImageView(),
		vk::ImageLayout::eShaderReadOnlyOptimal };
	writer.writeImage(1, bumpInfo);

	vk::DescriptorSet newSet; 
	writer.build(newSet);

	return newSet; 
}

void star::BumpMaterial::prep(StarDevice& device)
{
	this->TextureMaterial::prep(device);
	bumpMap->prepRender(device);
}
