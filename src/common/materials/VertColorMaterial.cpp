#include "VertColorMaterial.hpp"

void star::VertColorMaterial::prepRender(StarDevice& device)
{

}

void star::VertColorMaterial::initDescriptorLayouts(StarDescriptorSetLayout::Builder& constBuilder)
{
}

void star::VertColorMaterial::buildConstDescriptor(StarDescriptorWriter writer)
{
	writer.build(this->descriptorSet);
}

void star::VertColorMaterial::bind(vk::CommandBuffer& commandBuffer, vk::PipelineLayout pipelineLayout, int swapChainImageIndex)
{
	if (this->descriptorSet) {
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 2, 1, &this->descriptorSet, 0, nullptr);
	}
}