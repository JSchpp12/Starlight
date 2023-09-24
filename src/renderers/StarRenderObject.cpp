#include "StarRenderObject.hpp"

namespace star {
#pragma region Builder
	StarRenderObject::Builder::Builder(StarDevice& starDevice, GameObject& gameObject)
	: starDevice(starDevice), gameObject(gameObject) { }
	StarRenderObject::Builder& StarRenderObject::Builder::setNumFrames(size_t numImages) {
	this->numSwapChainImages = numImages;
	return *this;
}
	StarRenderObject::Builder& StarRenderObject::Builder::addMesh(std::unique_ptr<StarRenderMesh> renderMesh) {
	this->meshes.push_back(std::move(renderMesh));
	return *this;
}
std::unique_ptr<StarRenderObject> StarRenderObject::Builder::build() {
	assert(this->meshes.size() != 0 && "At least one mesh must be assigned to a render object");
	assert(this->numSwapChainImages > 0 && "Number of swap chain images must be set");

	return std::make_unique<StarRenderObject>(this->starDevice, this->gameObject, std::move(this->meshes), this->numSwapChainImages);
}
#pragma endregion 
void StarRenderObject::initDescriptorLayouts(StarDescriptorSetLayout::Builder& constBuilder) {
	//create
	for (auto& mesh : this->meshes) {
		mesh->initDescriptorLayouts(constBuilder);
	}
}

void StarRenderObject::initDescriptors(StarDescriptorSetLayout& constLayout, StarDescriptorPool& descriptorPool) {
	//add descriptors for each mesh -- right now this is the image descriptor
	for (auto& mesh : this->meshes) {

		mesh->initDescriptors(constLayout, descriptorPool);
	}
}

void StarRenderObject::render(vk::CommandBuffer& commandBuffer, vk::PipelineLayout& pipelineLayout, int swapChainIndexNum) {
	for (auto& mesh : this->meshes) {
		mesh->render(commandBuffer, pipelineLayout, swapChainIndexNum);
	}
}

std::vector<vk::DescriptorSet>& StarRenderObject::getDefaultDescriptorSets() {
	return this->uboDescriptorSets;
}
}