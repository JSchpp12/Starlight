#include "StarObject.hpp"

void star::StarObject::prepRender(star::StarDevice& device) {
	this->meshes = this->loadMeshes();

	for (auto& mesh : this->meshes) {
		mesh->prepRender(device);
	}
}

void star::StarObject::initDescriptorLayouts(StarDescriptorSetLayout::Builder& constLayout)
{
	for (auto& rmesh : this->meshes) {
		rmesh->initDescriptorLayouts(constLayout);
	}
}

void star::StarObject::initDescriptors(StarDevice& device, StarDescriptorSetLayout& constLayout, StarDescriptorPool& descriptorPool)
{
	//add descriptors for each mesh -- right now this is the image descriptor
	for (auto& rmesh : this->meshes) {
		rmesh->initDescriptors(device, constLayout, descriptorPool);
	}
}

void star::StarObject::render(vk::CommandBuffer& commandBuffer, vk::PipelineLayout& pipelineLayout, int swapChainIndexNum)
{
	for (auto& rmesh : this->meshes) {
		rmesh->render(commandBuffer, pipelineLayout, swapChainIndexNum);
	}
}