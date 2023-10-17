#include "BumpMaterial.hpp"

void star::BumpMaterial::prepRender(StarDevice& device)
{
	texture->prepRender(device); 
	bumpMap->prepRender(device); 
}

void star::BumpMaterial::initDescriptorLayouts(StarDescriptorSetLayout::Builder& constBuilder)
{
	constBuilder.addBinding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
	constBuilder.addBinding(2, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
}

void star::BumpMaterial::buildConstDescriptor(StarDescriptorWriter writer)
{
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

	writer.build(this->descriptorSet);
}

void star::BumpMaterial::bind(vk::CommandBuffer& commandBuffer, vk::PipelineLayout pipelineLayout, int swapChainImageIndex)
{
	if (this->descriptorSet) {
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 2, 1, &this->descriptorSet, 0, nullptr);
	}
}