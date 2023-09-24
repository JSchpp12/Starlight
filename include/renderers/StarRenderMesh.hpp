#pragma once 

#include "Mesh.hpp"
#include "Material.hpp"
#include "StarDescriptors.hpp"
#include "StarRenderMaterial.hpp"
#include "StarPipeline.hpp"
#include "StarDevice.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>

namespace star {
class StarRenderMesh {
public:
	class Builder {
	public:
		Builder(StarDevice& starDevice) : starDevice(starDevice) { }
		/// <summary>
		/// Set the Mesh object that the render object will be generated from 
		/// </summary>
		/// <param name="mesh">Target mesh object</param>
		/// <returns></returns>
		Builder& setMesh(Mesh& mesh);
		/// <summary>
		/// Set properties relevant to rendering operations. 
		/// </summary>
		/// <param name="vertexBufferOffset">Offset within the vertex buffer that will be used when submitting draw commands</param>
		/// <returns></returns>
		Builder& setRenderSettings(uint32_t vertexBufferOffset);
		Builder& setMaterial(std::unique_ptr<StarRenderMaterial> renderMaterial);
		std::unique_ptr<StarRenderMesh> build();

	private:
		StarDevice& starDevice;
		std::unique_ptr<StarRenderMaterial> renderMaterial;
		Mesh* mesh = nullptr;
		uint32_t beginIndex = 0;

	};

	StarRenderMesh(StarDevice& starDevice, Mesh& mesh, std::unique_ptr<StarRenderMaterial> material, uint32_t vbOffset = 0)
		: starDevice(starDevice), mesh(mesh), renderMaterial(std::move(material)) {
		this->startIndex = vbOffset;
	}
	void initDescriptorLayouts(StarDescriptorSetLayout::Builder& constBuilder);
	/// <summary>
	/// Init object with needed structures such as descriptors
	/// </summary>
	/// <param name="constBuilder">Writer to use when creating static descriptors (those updated once on init)</param>
	void initDescriptors(StarDescriptorSetLayout& constLayout, StarDescriptorPool& descriptorPool);
	void render(vk::CommandBuffer& commandBuffer, vk::PipelineLayout& pipelineLayout, int swapChainImageIndex);

	void setVBOffset(uint32_t offset) { this->startIndex = offset; }
	StarRenderMaterial& getStarRenderMaterial() { return *this->renderMaterial; }
	Mesh& getMesh() { return this->mesh; }
	vk::DescriptorSet& getDescriptor() { return this->renderMaterial->getDescriptor(); }

private:
	StarDevice& starDevice;
	Mesh& mesh;
	std::unique_ptr<StarRenderMaterial> renderMaterial;
	uint32_t startIndex;								//index in vertex buffer where draw will begin             

};
}