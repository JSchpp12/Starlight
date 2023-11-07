#include "HeightDisplacementMaterial.hpp"

void star::HeightDisplacementMaterial::prep(StarDevice& device)
{
	displaceTexture->prepRender(device);
}

void star::HeightDisplacementMaterial::getDescriptorSetLayout(StarDescriptorSetLayout::Builder& constBuilder)
{
	constBuilder.addBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
}

void star::HeightDisplacementMaterial::cleanup(StarDevice& device)
{
	this->displaceTexture.reset(); 
}

vk::DescriptorSet star::HeightDisplacementMaterial::buildDescriptorSet(StarDevice& device, StarDescriptorSetLayout& groupLayout, StarDescriptorPool& groupPool)
{
	auto writer = StarDescriptorWriter(device, groupLayout, groupPool);

	auto texInfo = vk::DescriptorImageInfo{
		displaceTexture->getSampler(),
		displaceTexture->getImageView(),
		vk::ImageLayout::eShaderReadOnlyOptimal
	};

	writer.writeImage(0, texInfo);
	vk::DescriptorSet set;
	writer.build(set);

	return set; 
}
