#include "StarObject.hpp"

void star::StarObject::prepRender(star::StarDevice& device) {
	for (auto& mesh : this->getMeshes()) {
		mesh->prepRender(device);
	}
}

void star::StarObject::initDescriptorLayouts(star::StarDescriptorSetLayout::Builder& constLayout) {
	for (auto& rmesh : this->getMeshes()) {
		rmesh->initDescriptorLayouts(constLayout);
	}
}

void star::StarObject::initDescriptors(star::StarDevice& device, star::StarDescriptorSetLayout& constLayout, StarDescriptorPool& descriptorPool) {
	//add descriptors for each mesh -- right now this is the image descriptor
	for (auto& rmesh : this->getMeshes()) {
		rmesh->initDescriptors(device, constLayout, descriptorPool);
	}
}

void star::StarObject::render(vk::CommandBuffer& commandBuffer, vk::PipelineLayout& pipelineLayout, int swapChainIndexNum) {
	for (auto& rmesh : this->getMeshes()) {
		rmesh->render(commandBuffer, pipelineLayout, swapChainIndexNum);
	}
}