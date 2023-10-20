#include "HeightDisplacementMaterial.hpp"

void star::HeightDisplacementMaterial::prepRender(StarDevice& device)
{
	displaceTexture.prepRender(device);
}

void star::HeightDisplacementMaterial::initDescriptorLayouts(StarDescriptorSetLayout::Builder& constBuilder)
{
	constBuilder.addBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
}

void star::HeightDisplacementMaterial::buildConstDescriptor(StarDescriptorWriter writer)
{
	auto texInfo = vk::DescriptorImageInfo{
		displaceTexture.getSampler(),
		displaceTexture.getImageView(),
		vk::ImageLayout::eShaderReadOnlyOptimal 
	};

	writer.writeImage(0, texInfo);
	writer.build(this->descriptorSet);
}

void star::HeightDisplacementMaterial::bind(vk::CommandBuffer& commandBuffer, vk::PipelineLayout pipelineLayout, int swapChainImageIndex)
{
	if (this->descriptorSet) {
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 2, 1, &this->descriptorSet, 0, nullptr);
	}
}
