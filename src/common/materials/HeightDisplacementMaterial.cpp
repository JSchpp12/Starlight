#include "HeightDisplacementMaterial.hpp"

void star::HeightDisplacementMaterial::prep(StarDevice& device)
{
	texture->prepRender(device);
}

void star::HeightDisplacementMaterial::applyDescriptorSetLayouts(star::StarDescriptorSetLayout::Builder& constBuilder)
{
	constBuilder.addBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
}

void star::HeightDisplacementMaterial::cleanup(StarDevice& device)
{
	this->texture.reset();
}

vk::DescriptorSet star::HeightDisplacementMaterial::buildDescriptorSet(StarDevice& device, StarDescriptorSetLayout& groupLayout, StarDescriptorPool& groupPool, const int& frameInFlightIndex)
{
	auto writer = StarDescriptorWriter(device, groupLayout, groupPool);

	auto texInfo = vk::DescriptorImageInfo{
		texture->getSampler(),
		texture->getImageView(),
		vk::ImageLayout::eShaderReadOnlyOptimal
	};

	writer.writeImage(0, texInfo);
	vk::DescriptorSet set = writer.build();

	return set;
}
