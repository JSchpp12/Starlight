#include "StarMesh.hpp"

void star::StarMesh::prepRender(star::StarDevice& device)
{
	this->material->prepRender(device); 
}

void star::StarMesh::recordCommands(vk::CommandBuffer& commandBuffer, vk::PipelineLayout& pipelineLayout, int swapChainImageIndex, uint32_t vb_start, uint32_t ib_start) {
	this->material->bind(commandBuffer, pipelineLayout, swapChainImageIndex);

	uint32_t vertexCount = CastHelpers::size_t_to_unsigned_int(this->vertices->size()); 
	uint32_t indexCount = CastHelpers::size_t_to_unsigned_int(this->indices->size());
	//commandBuffer.drawIndexed(3, 0, ib_start, vb_start, 0);
	commandBuffer.draw(3, 0, 0, 0); 
}