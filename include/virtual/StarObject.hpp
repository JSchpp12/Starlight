#pragma once

#include "ManagerRenderResource.hpp"
#include "ConfigFile.hpp"
#include "StarDevice.hpp"
#include "StarEntity.hpp"
#include "StarMaterial.hpp"
#include "StarShader.hpp"
#include "StarPipeline.hpp"
#include "StarMesh.hpp"
#include "StarObjectInstance.hpp"
#include "StarShaderInfo.hpp"
#include "StarGraphicsPipeline.hpp"
#include "StarCommandBuffer.hpp"
#include "ManagerDescriptorPool.hpp"
#include "RenderingTargetInfo.hpp"
#include "DescriptorModifier.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#define GLM_ENABLE_EXPERIMENTAL
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
	class StarObject : private DescriptorModifier {
	public:
		bool drawNormals = false;
		bool drawBoundingBox = false; 
		bool isVisible = true; 

		static void initSharedResources(StarDevice& device, vk::Extent2D swapChainExtent,
			int numSwapChainImages, StarDescriptorSetLayout& globalDescriptors, 
			RenderingTargetInfo renderingInfo);

		static void cleanupSharedResources(StarDevice& device);

		/// <summary>
		/// Create an object from manually defined/generated mesh structures
		/// </summary>
		/// <param name="meshes"></param>
		StarObject(); 

		virtual ~StarObject(){}

		virtual void cleanupRender(StarDevice& device); 

		virtual std::unique_ptr<StarPipeline> buildPipeline(StarDevice& device,
			vk::Extent2D swapChainExtent, vk::PipelineLayout pipelineLayout, 
			RenderingTargetInfo renderInfo);

		/// <summary>
		/// Prepare needed objects for rendering operations.
		/// </summary>
		/// <param name="device"></param>
		virtual void prepRender(star::StarDevice& device, vk::Extent2D swapChainExtent,
			vk::PipelineLayout pipelineLayout, RenderingTargetInfo renderingInfo, int numSwapChainImages, 
			StarShaderInfo::Builder fullEngineBuilder);

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
			StarPipeline& sharedPipeline, star::StarShaderInfo::Builder fullEngineBuilder);

		virtual void recordPreRenderPassCommands(vk::CommandBuffer& commandBuffer, int swapChainIndexNum) {};

		/// <summary>
		/// Create render call
		/// </summary>
		/// <param name="commandBuffer"></param>
		/// <param name="pipelineLayout"></param>
		/// <param name="swapChainIndexNum"></param>
		virtual void recordRenderPassCommands(vk::CommandBuffer& commandBuffer, vk::PipelineLayout& pipelineLayout, int swapChainIndexNum);

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
		virtual std::vector<std::shared_ptr<star::StarDescriptorSetLayout>> getDescriptorSetLayouts(StarDevice& device);

		virtual void prepareDescriptors(star::StarDevice& device, int numSwapChainImages,
			StarShaderInfo::Builder engineInfoBuilder);

#pragma region getters
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
		std::vector<Handle> instanceNormalInfos; 
		std::vector<Handle> instanceModelInfos;
		std::vector<std::unique_ptr<StarMesh>> meshes;
		std::vector<std::unique_ptr<StarObjectInstance>> instances; 

		std::unique_ptr<StarShaderInfo::Builder> engineBuilder;
		
		void prepareMeshes(star::StarDevice& device); 

		virtual void createInstanceBuffers(star::StarDevice& device, int numImagesInFlight);

		virtual void createBoundingBox(std::vector<Vertex>& verts, std::vector<uint32_t>& inds);

		// Inherited via DescriptorModifier
		virtual std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(const int& numFramesInFlight) override;
		virtual void createDescriptors(star::StarDevice& device, const int& numFramesInFlight) override;

	private:
		static std::unique_ptr<StarDescriptorSetLayout> instanceDescriptorLayout;
		static vk::PipelineLayout extrusionPipelineLayout;
		static std::unique_ptr<StarGraphicsPipeline> tri_normalExtrusionPipeline, triAdj_normalExtrusionPipeline;
		static std::unique_ptr<StarDescriptorSetLayout> boundDescriptorLayout;
		static vk::PipelineLayout boundPipelineLayout;
		static std::unique_ptr<StarGraphicsPipeline> boundBoxPipeline;

		std::unique_ptr<std::vector<std::reference_wrapper<StarDescriptorSetLayout>>> groupLayout;
		std::unique_ptr<std::vector<std::vector<vk::DescriptorSet>>> globalSets;

		Handle boundingBoxVertBuffer, boundingBoxIndexBuffer;
		std::vector<std::vector<vk::DescriptorSet>> boundingDescriptors; 
		std::unique_ptr<BufferHandle> vertBuffer, indBuffer; 
		uint32_t boundingBoxIndsCount = 0; 

		void recordDrawCommandNormals(vk::CommandBuffer& commandBuffer);

		void recordDrawCommandBoundingBox(vk::CommandBuffer& commandBuffer, int inFlightIndex);;

		void calculateBoundingBox(std::vector<Vertex>& verts, std::vector<uint32_t>& inds);
};
}