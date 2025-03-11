#include "StarMesh.hpp"

void star::StarMesh::prepRender(star::StarDevice& device)
{
	this->material->prepRender(device);
}

void star::StarMesh::recordRenderPassCommands(vk::CommandBuffer& commandBuffer, vk::PipelineLayout& pipelineLayout, int& frameInFlightIndex, const uint32_t& instanceCount){

	this->material->bind(commandBuffer, pipelineLayout, frameInFlightIndex);

	vk::DeviceSize offset{0};
	auto& vBuff = ManagerRenderResource::getBuffer(this->vertBuffer);
	auto& iBuff = ManagerRenderResource::getBuffer(this->indBuffer);
	commandBuffer.bindVertexBuffers(0,vBuff.getVulkanBuffer(), offset);
	commandBuffer.bindIndexBuffer(iBuff.getVulkanBuffer(), offset, vk::IndexType::eUint32);

	this->material->bind(commandBuffer, pipelineLayout, frameInFlightIndex);

	commandBuffer.drawIndexed(this->numInds, instanceCount, 0, 0, 0);
}