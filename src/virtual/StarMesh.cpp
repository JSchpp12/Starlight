#include "StarMesh.hpp"

void star::StarMesh::prepRender(star::StarDevice& device)
{
	this->material->prepareRender(device);
}

void star::StarMesh::recordCommands(vk::CommandBuffer& commandBuffer, vk::PipelineLayout& pipelineLayout, int swapChainImageIndex, uint32_t vb_start, uint32_t ib_start) {
	this->material->bind(commandBuffer, pipelineLayout, swapChainImageIndex);

	uint32_t vertexCount = CastHelpers::size_t_to_unsigned_int(this->vertices->size()); 
	uint32_t indexCount = CastHelpers::size_t_to_unsigned_int(this->indices->size());
	commandBuffer.drawIndexed(indexCount, 1, ib_start, 0, 0);
}