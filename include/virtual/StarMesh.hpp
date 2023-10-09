#pragma once 

#include "StarMaterial.hpp"
#include "StarDevice.hpp"
#include "StarDescriptors.hpp"
#include "Triangle.hpp"

#include <vulkan/vulkan.hpp>

namespace star {
	class StarMesh {
	public:
		StarMesh(std::unique_ptr<std::vector<Triangle>> triangles, std::unique_ptr<StarMaterial> material, 
			uint32_t vbOffset = 0) : triangles(std::move(triangles)), material(std::move(material)),
			vbOffset(vbOffset) {};

		virtual void prepRender(StarDevice& device); 

		virtual void initDescriptorLayouts(StarDescriptorSetLayout::Builder& constBuilder); 

		virtual void initDescriptors(StarDevice& device, StarDescriptorSetLayout& constLayout, StarDescriptorPool& descriptorPool);

		virtual void render(vk::CommandBuffer& commandBuffer, vk::PipelineLayout& pipelineLayout, int swapChainImageIndex); 

		std::vector<Triangle>& getTriangles() { return *this->triangles; }
	protected:
		std::unique_ptr<std::vector<Triangle>> triangles; 
		std::unique_ptr<StarMaterial> material; 
		uint32_t vbOffset; 
	};
}