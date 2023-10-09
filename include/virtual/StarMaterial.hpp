#pragma once

#include "StarDevice.hpp"
#include "StarTexture.hpp"

#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

#include <string>

namespace star {
	class StarMaterial {
	public:
		StarMaterial() = default; 
		virtual ~StarMaterial() = default; 

		/// <summary>
		/// Function which should contain processes to create all needed functionalities
		/// for rendering operations. Example is creating needed rendering textures for 
		/// and gpu memory. 
		/// </summary>
		/// <param name="device">Device that is being used in rendering operations</param>
		virtual void prepRender(StarDevice& device) = 0; 

		/// <summary>
		/// Initalize the const descriptor set layouts with needed descriptor slots
		/// </summary>
		/// <param name="constBuilder"></param>
		virtual void initDescriptorLayouts(StarDescriptorSetLayout::Builder& constBuilder) = 0;

		/// <summary>
		/// Init Render Material with proper descriptors
		/// </summary>
		/// <param name="staticDescriptorSetLayout">DescriptorSetLayout to be used when creating object descriptors which are updated once (during init)</param>
		virtual void buildConstDescriptor(StarDescriptorWriter writer) = 0;

		virtual void bind(vk::CommandBuffer& commandBuffer, vk::PipelineLayout pipelineLayout, int swapChainImageIndex)=0; 

#pragma region getters
		vk::DescriptorSet& getDescriptorSet() { return this->descriptorSet; }
#pragma endregion
	protected:
		vk::DescriptorSet descriptorSet; 

	private: 

	};
}