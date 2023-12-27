#pragma once

#include "ConfigFile.hpp"
#include "StarDevice.hpp"
#include "StarEntity.hpp"
#include "StarDescriptors.hpp"
#include "StarMaterial.hpp"
#include "StarShader.hpp"
#include "StarPipeline.hpp"
#include "StarMesh.hpp"
#include "StarObjectInstance.hpp"
#include "StarGraphicsPipeline.hpp"
#include "StarCommandBuffer.hpp"
#include "ManagerDescriptorPool.hpp"
#include "RenderResourceModifier.hpp"

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
	class StarObject : private RenderResourceModifier {
	public:
		bool drawNormals = false;

		static void initSharedResources(StarDevice& device, vk::Extent2D swapChainExtent,
			vk::RenderPass renderPass, int numSwapChainImages,
			StarDescriptorSetLayout& globalDescriptors);

		static void cleanupSharedResources(StarDevice& device);

		/// <summary>
		/// Create an object from manually defined/generated mesh structures
		/// </summary>
		/// <param name="meshes"></param>
		StarObject() = default; 

		virtual ~StarObject(){}

		virtual void cleanupRender(StarDevice& device); 

		virtual std::unique_ptr<StarPipeline> buildPipeline(StarDevice& device,
			vk::Extent2D swapChainExtent, vk::PipelineLayout pipelineLayout, 
			vk::RenderPass renderPass);

		/// <summary>
		/// Prepare needed objects for rendering operations.
		/// </summary>
		/// <param name="device"></param>
		virtual void prepRender(star::StarDevice& device, vk::Extent2D swapChainExtent,
			vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass, int numSwapChainImages, 
			std::vector<std::reference_wrapper<StarDescriptorSetLayout>> groupLayout, 
			std::vector<std::vector<vk::DescriptorSet>> globalSets);

		/// <summary>
		/// Initalize this object. This object wil not have its own pipeline. It will expect to share one.
		/// </summary>
		/// <param name="device"></param>
		/// <param name="numSwapChainImages"></param>
		/// <param name="groupLayout"></param>
		/// <param name="groupPool"></param>
		/// <param name="globalSets"></param>
		/// <param name="sharedPipeline"></param>
		virtual void prepRender(star::StarDevice& device, int numSwapChainImages,
			std::vector<std::reference_wrapper<StarDescriptorSetLayout>> groupLayout,
			std::vector<std::vector<vk::DescriptorSet>> globalSets, StarPipeline& sharedPipeline);

		virtual void recordPreRenderPassCommands(StarCommandBuffer& commandBuffer, int swapChainIndexNum) {};

		/// <summary>
		/// Create render call
		/// </summary>
		/// <param name="commandBuffer"></param>
		/// <param name="pipelineLayout"></param>
		/// <param name="swapChainIndexNum"></param>
		virtual void recordRenderPassCommands(StarCommandBuffer& commandBuffer, vk::PipelineLayout& pipelineLayout, int swapChainIndexNum, uint32_t vb_start, uint32_t ib_start);

		/// @brief Create an instance of this object.
		/// @return A reference to the created instance. The object will own the instance. 
		virtual StarObjectInstance& createInstance(); 

		/// <summary>
		/// Runtime update to allow object to update anything it needs to prepare for the next 
		/// main draw command.
		/// </summary>
		virtual void prepDraw(int swapChainTarget); 

		/// <summary>
		/// Every material must provide a method to return shaders within a map. The keys of the map will contain the stages in which the 
		/// corresponding shader will be used. 
		/// </summary>
		/// <returns></returns>
		virtual std::unordered_map<star::Shader_Stage, StarShader> getShaders() = 0;

		/// @brief Create descriptor set layouts for this object. 
		/// @param device 
		/// @return 
		virtual std::vector<std::unique_ptr<star::StarDescriptorSetLayout>> getDescriptorSetLayouts(StarDevice& device);

#pragma region getters
		//glm::mat4 getNormalMatrix() { return glm::inverseTranspose(getDisplayMatrix()); }
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
		std::unique_ptr<StarPipeline> normalExtrusionPipeline; 
		std::unique_ptr<StarDescriptorSetLayout> setLayout; 
		std::vector<std::vector<std::unique_ptr<StarBuffer>>> instanceUniformBuffers; 
		std::vector<std::unique_ptr<StarMesh>> meshes;
		std::vector<std::unique_ptr<StarObjectInstance>> instances; 
		
		void prepareMeshes(star::StarDevice& device); 

		virtual void prepareDescriptors(star::StarDevice& device, int numSwapChainImages, 
			std::vector<std::reference_wrapper<StarDescriptorSetLayout>> groupLayout, 
			std::vector<std::vector<vk::DescriptorSet>> globalSets);

		virtual void createInstanceBuffers(star::StarDevice& device, int numImagesInFlight); 

		void recordDrawCommandNormals(star::StarCommandBuffer& commandBuffer, uint32_t ib_start, int inFlightIndex); 

		virtual void initResources(int numFramesInFlight) override;

	private:
		static std::unique_ptr<StarDescriptorSetLayout> instanceDescriptorLayout; 
		static vk::PipelineLayout extrusionPipelineLayout; 
		static std::unique_ptr<StarGraphicsPipeline> tri_normalExtrusionPipeline, triAdj_normalExtrusionPipeline; 
};
}