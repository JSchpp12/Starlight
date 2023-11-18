#pragma once

#include "StarDevice.hpp"
#include "StarEntity.hpp"
#include "StarDescriptors.hpp"
#include "StarMaterial.hpp"
#include "StarShader.hpp"
#include "StarPipeline.hpp"
#include "StarMesh.hpp"
#include "StarGraphicsPipeline.hpp"
#include "StarCommandBuffer.hpp"
#include "ManagerDescriptorPool.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_cross_product.hpp>

#include <vulkan/vulkan.hpp>

#include <optional>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace star {
	/// <summary>
	/// Base class for renderable objects.
	/// </summary>
	class StarObject : public StarEntity {
	public:
		/// <summary>
		/// Create an object from manually defined/generated mesh structures
		/// </summary>
		/// <param name="meshes"></param>
		StarObject() = default; 

		virtual ~StarObject(){}

		virtual void cleanupRender(StarDevice& device); 

		virtual std::unique_ptr<StarPipeline> buildPipeline(StarDevice& device,
			vk::Extent2D swapChainExtent, vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass);

		/// <summary>
		/// Prepare memory needed for render operations. 
		/// </summary>
		virtual void initRender(int numFramesInFlight);

		/// <summary>
		/// Prepare needed objects for rendering operations.
		/// </summary>
		/// <param name="device"></param>
		virtual void prepRender(star::StarDevice& device, vk::Extent2D swapChainExtent,
			vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass, int numSwapChainImages, StarDescriptorSetLayout& groupLayout,
			StarDescriptorPool& groupPool, std::vector<std::vector<vk::DescriptorSet>> globalSets);

		/// <summary>
		/// Initalize this object. This object wil not have its own pipeline. It will expect to share one.
		/// </summary>
		/// <param name="device"></param>
		/// <param name="numSwapChainImages"></param>
		/// <param name="groupLayout"></param>
		/// <param name="groupPool"></param>
		/// <param name="globalSets"></param>
		/// <param name="sharedPipeline"></param>
		virtual void prepRender(star::StarDevice& device, int numSwapChainImages, StarDescriptorSetLayout& groupLayout,
			StarDescriptorPool& groupPool, std::vector<std::vector<vk::DescriptorSet>> globalSets, StarPipeline& sharedPipeline);

		/// <summary>
		/// Create render call
		/// </summary>
		/// <param name="commandBuffer"></param>
		/// <param name="pipelineLayout"></param>
		/// <param name="swapChainIndexNum"></param>
		virtual void recordCommands(StarCommandBuffer& commandBuffer, vk::PipelineLayout& pipelineLayout, int swapChainIndexNum, uint32_t vb_start, uint32_t ib_start);

		/// <summary>
		/// Runtime update to allow object to update anything it needs to prepare for the next 
		/// main draw command.
		/// </summary>
		virtual void prepDraw(int swapChainTarget) { }

		/// <summary>
		/// Every material must provide a method to return shaders within a map. The keys of the map will contain the stages in which the 
		/// corresponding shader will be used. 
		/// </summary>
		/// <returns></returns>
		virtual std::unordered_map<star::Shader_Stage, StarShader> getShaders() = 0;

#pragma region getters
		glm::mat4 getNormalMatrix() { return glm::inverseTranspose(getDisplayMatrix()); }
		const std::vector<std::unique_ptr<StarMesh>>& getMeshes() { return this->meshes; }
		virtual StarPipeline& getPipline() {
			assert(this->pipeline && "This object is expecting to share a pipeline with another object."); 
			return *this->pipeline; 
		}
#pragma endregion

	protected:
		///pipeline + rendering infos
		StarPipeline* sharedPipeline = nullptr;
		std::unique_ptr<StarPipeline> pipeline; 

		std::vector<std::unique_ptr<StarMesh>> meshes;

		void prepareMeshes(star::StarDevice& device); 

		void prepareDescriptors(star::StarDevice& device, int numSwapChainImages, 
			StarDescriptorSetLayout& groupLayout, StarDescriptorPool& groupPool, std::vector<std::vector<vk::DescriptorSet>> globalSets);
	};
}