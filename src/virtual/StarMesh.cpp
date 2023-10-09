#include "StarMesh.hpp"

void star::StarMesh::prepRender(star::StarDevice& device) 
{
	this->material->prepRender(device); 
}

void star::StarMesh::initDescriptorLayouts(star::StarDescriptorSetLayout::Builder& constBuilder) 
{
	this->material->initDescriptorLayouts(constBuilder); 
}

void star::StarMesh::initDescriptors(StarDevice& device, star::StarDescriptorSetLayout& constLayout, star::StarDescriptorPool& descriptorPool) 
{
	auto writer = StarDescriptorWriter(device, constLayout, descriptorPool); 

	material->buildConstDescriptor(writer); 
}

void star::StarMesh::render(vk::CommandBuffer& commandBuffer, vk::PipelineLayout& pipelineLayout, int swapChainImageIndex) {
	this->material->bind(commandBuffer, pipelineLayout, swapChainImageIndex);

	uint32_t vertCount = CastHelpers::size_t_to_unsigned_int(this->indices->size());
	commandBuffer.drawIndexed(vertCount, 1, 0, this->vbOffset, 0);
}