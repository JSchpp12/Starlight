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
std::unique_ptr<StarRenderMesh> StarRenderMesh::Builder::build() {
	assert(this->mesh != nullptr && "A mesh and render material are required to create a StarRenderMesh object");

	return std::make_unique<StarRenderMesh>(this->starDevice, *this->mesh, this->beginIndex);
}
#pragma endregion 
void StarRenderMesh::prepRender(StarDevice& device) {
	mesh.getMaterial().prepRender(device); 
}

void StarRenderMesh::initDescriptorLayouts(StarDescriptorSetLayout::Builder& constBuilder) {
	this->mesh.getMaterial().initDescriptorLayouts(constBuilder); 
}

void StarRenderMesh::initDescriptors(StarDescriptorSetLayout& constLayout, StarDescriptorPool& descriptorPool) {
	auto writer = StarDescriptorWriter(this->starDevice, constLayout, descriptorPool);

	//add per object const descriptors

	//build per mesh descriptors
	this->mesh.getMaterial().buildConstDescriptor(writer);
}
void StarRenderMesh::render(vk::CommandBuffer& commandBuffer, vk::PipelineLayout& pipelineLayout, int swapChainImageIndex) {
	this->mesh.getMaterial().bind(commandBuffer, pipelineLayout, swapChainImageIndex); 

	auto testTriangle = this->mesh.getTriangles()->size() * 3;
	commandBuffer.drawIndexed(testTriangle, 1, 0, this->startIndex, 0);
}
}