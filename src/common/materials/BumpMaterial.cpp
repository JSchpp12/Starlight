#include "BumpMaterial.hpp"

void star::BumpMaterial::prepRender(StarDevice& device)
{
	renderTexture = std::unique_ptr<StarTexture>(new StarTexture(device, this->texture)); 
	renderBumpMap = std::unique_ptr<StarTexture>(new StarTexture(device, this->bumpMap)); 
}

void star::BumpMaterial::initDescriptorLayouts(StarDescriptorSetLayout::Builder& constBuilder)
{
	constBuilder.addBinding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
	constBuilder.addBinding(2, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
}

void star::BumpMaterial::buildConstDescriptor(StarDescriptorWriter writer)
{
	auto texInfo = vk::DescriptorImageInfo{
		renderTexture->getSampler(),
		renderTexture->getImageView(),
		vk::ImageLayout::eShaderReadOnlyOptimal };
	writer.writeImage(0, texInfo);

	auto bumpInfo = vk::DescriptorImageInfo{
		renderBumpMap->getSampler(),
		renderBumpMap->getImageView(),
		vk::ImageLayout::eShaderReadOnlyOptimal };
	writer.writeImage(1, bumpInfo);

	writer.build(this->descriptorSet);
}

void star::BumpMaterial::bind(vk::CommandBuffer& commandBuffer, vk::PipelineLayout pipelineLayout, int swapChainImageIndex)
{
}