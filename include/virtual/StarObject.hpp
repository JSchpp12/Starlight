#pragma once

#include "BumpMaterial.hpp"

#include "StarEntity.hpp"
#include "StarDescriptors.hpp"
#include "StarMaterial.hpp"
#include "StarMesh.hpp"
#include "StarTexture.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_cross_product.hpp>

#include <vulkan/vulkan.hpp>

#include <memory>
#include <string>
#include <vector>

namespace star {
	/// <summary>
	/// Base class for renderable objects.
	/// </summary>
	class StarObject : public StarEntity{
	public:
		/// <summary>
		/// Create an object from manually defined/generated mesh structures
		/// </summary>
		/// <param name="meshes"></param>
		StarObject()
			: uboDescriptorSets(3) {
			vertShader.shaderStage = Shader_Stage::vertex; 
			fragShader.shaderStage = Shader_Stage::fragment;
		};

		virtual ~StarObject() = default;

		/// <summary>
		/// Prepare needed objects for rendering operations.
		/// </summary>
		/// <param name="device"></param>
		virtual void prepRender(StarDevice& device)=0;

		///// <summary>
		///// Function which is called before render pass. Should be used to update buffers.
		///// </summary>
		//virtual void update() = 0;

		virtual void initDescriptorLayouts(StarDescriptorSetLayout::Builder& constLayout)=0;

		/// <summary>
		/// Init object with needed descriptors
		/// </summary>
		/// <param name="descriptorWriter"></param>
		virtual void initDescriptors(StarDevice& device, StarDescriptorSetLayout& constLayout, StarDescriptorPool& descriptorPool)=0;

		/// <summary>
		/// Create render call
		/// </summary>
		/// <param name="commandBuffer"></param>
		/// <param name="pipelineLayout"></param>
		/// <param name="swapChainIndexNum"></param>
		virtual void render(vk::CommandBuffer& commandBuffer, vk::PipelineLayout& pipelineLayout, int swapChainIndexNum)=0; 

#pragma region getters
		//get the handle for the vertex shader 
		Handle getVertShader() { return vertShader; }
		//get the handle for the fragment shader
		Handle getFragShader() { return fragShader; }
		glm::mat4 getNormalMatrix() { return glm::inverseTranspose(getDisplayMatrix()); }
		virtual const std::vector<std::unique_ptr<StarMesh>>& getMeshes() = 0; 
		virtual std::vector<vk::DescriptorSet>& getDefaultDescriptorSets() { return this->uboDescriptorSets; }
#pragma endregion

	protected:
		std::vector<vk::DescriptorSet> uboDescriptorSets;
		Handle vertShader = Handle::getDefault(), fragShader = Handle::getDefault();
	};
}