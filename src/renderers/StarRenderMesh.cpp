#include "StarRenderMesh.hpp"

namespace star {
#pragma region builder 
StarRenderMesh::Builder& StarRenderMesh::Builder::setMesh(Mesh& mesh) {
	this->mesh = &mesh;
	return *this;
}
StarRenderMesh::Builder& StarRenderMesh::Builder::setRenderSettings(uint32_t vertexBufferOffset) {
	this->beginIndex = vertexBufferOffset;
	return *this;
}
StarRenderMesh::Builder& StarRenderMesh::Builder::setMaterial(std::unique_ptr<StarRenderMaterial> renderMaterial) {
	this->renderMaterial = std::move(renderMaterial);
	return *this;
}
std::unique_ptr<StarRenderMesh> StarRenderMesh::Builder::build() {
	assert(this->mesh != nullptr && this->renderMaterial && "A mesh and render material are required to create a StarRenderMesh object");

	return std::make_unique<StarRenderMesh>(this->starDevice, *this->mesh, std::move(this->renderMaterial), this->beginIndex);
}
#pragma endregion 
void StarRenderMesh::initDescriptorLayouts(StarDescriptorSetLayout::Builder& constBuilder) {
	this->renderMaterial->initDescriptorLayouts(constBuilder);
}
void StarRenderMesh::initDescriptors(StarDescriptorSetLayout& constLayout, StarDescriptorPool& descriptorPool) {
	auto writer = StarDescriptorWriter(this->starDevice, constLayout, descriptorPool);

	//add per object const descriptors

	//build per mesh descriptors
	this->renderMaterial->buildConstDescriptor(writer);
}
void StarRenderMesh::render(vk::CommandBuffer& commandBuffer, vk::PipelineLayout& pipelineLayout, int swapChainImageIndex) {
	this->renderMaterial->bind(commandBuffer, pipelineLayout, swapChainImageIndex);

	auto testTriangle = this->mesh.getTriangles()->size() * 3;
	commandBuffer.drawIndexed(testTriangle, 1, 0, this->startIndex, 0);
}
}