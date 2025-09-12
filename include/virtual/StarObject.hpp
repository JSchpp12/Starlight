#pragma once

#include "ConfigFile.hpp"
#include "DeviceContext.hpp"
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
#include "DescriptorModifier.hpp"
#include "core/renderer/RenderingContext.hpp"

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

		static void initSharedResources(core::device::DeviceContext& device, vk::Extent2D swapChainExtent,
			int numSwapChainImages, StarDescriptorSetLayout& globalDescriptors, 
			core::renderer::RenderingTargetInfo renderingInfo);

		static void cleanupSharedResources(core::device::DeviceContext& device);

		/// <summary>
		/// Create an object from manually defined/generated mesh structures
		/// </summary>
		/// <param name="meshes"></param>
		StarObject() = default; 

		virtual ~StarObject() = default;

		virtual void cleanupRender(core::device::DeviceContext& device); 

		virtual Handle buildPipeline(core::device::DeviceContext& device,
			vk::Extent2D swapChainExtent, vk::PipelineLayout pipelineLayout, 
			core::renderer::RenderingTargetInfo renderInfo);

		virtual void prepRender(star::core::device::DeviceContext& context, vk::Extent2D swapChainExtent,
			vk::PipelineLayout pipelineLayout, core::renderer::RenderingTargetInfo renderingInfo, int numSwapChainImages, 
			StarShaderInfo::Builder fullEngineBuilder);

		virtual core::renderer::RenderingContext buildRenderingContext(star::core::device::DeviceContext &context); 

		virtual void prepRender(star::core::device::DeviceContext& context, int numSwapChainImages, 
			Handle sharedPipeline, star::StarShaderInfo::Builder fullEngineBuilder);

		///Function to contain any commands to be submitted before the start of the rendering pass this object is contained in begins
		virtual void recordPreRenderPassCommands(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex) {};

		///Function to contain any commands to be submitted after the end of the rendering pass this object is contained in
		virtual void recordPostRenderPassCommands(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex) {}; 

		/// <summary>
		/// Create render call
		/// </summary>
		/// <param name="commandBuffer"></param>
		/// <param name="pipelineLayout"></param>
		/// <param name="swapChainIndexNum"></param>
		void recordRenderPassCommands(vk::CommandBuffer& commandBuffer, vk::PipelineLayout& pipelineLayout, int swapChainIndexNum);

		/// @brief Create an instance of this object.
		/// @return A reference to the created instance. The object will own the instance. 
		virtual StarObjectInstance& createInstance(); 

		/// <summary>
		/// Runtime update to allow object to update anything it needs to prepare for the next 
		/// main draw command.
		/// </summary>
		virtual void prepDraw(int swapChainTarget); 

		virtual void frameUpdate(core::device::DeviceContext &context); 

		/// <summary>
		/// Every material must provide a method to return shaders within a map. The keys of the map will contain the stages in which the 
		/// corresponding shader will be used. 
		/// </summary>
		/// <returns></returns>
		virtual std::unordered_map<star::Shader_Stage, StarShader> getShaders() = 0;

		/// @brief Create descriptor set layouts for this object. 
		/// @param device 
		/// @return 
		virtual std::vector<std::shared_ptr<star::StarDescriptorSetLayout>> getDescriptorSetLayouts(core::device::DeviceContext& device);

		virtual void prepareDescriptors(star::core::device::DeviceContext& device, int numSwapChainImages,
			StarShaderInfo::Builder engineInfoBuilder);

#pragma region getters
		const std::vector<std::unique_ptr<StarMesh>>& getMeshes() { return this->meshes; }
		Handle getPipline() {
			return this->pipeline; 
		}
#pragma endregion

	protected:
		///pipeline + rendering infos
		// Pipeline* sharedPipeline = nullptr;
		Handle pipeline; 
		Handle sharedPipeline;
		std::unique_ptr<core::renderer::RenderingContext> renderingContext; 
		std::unique_ptr<StarPipeline> normalExtrusionPipeline; 
		std::unique_ptr<StarDescriptorSetLayout> setLayout; 
		std::vector<Handle> instanceNormalInfos; 
		std::vector<Handle> instanceModelInfos;
		std::vector<std::unique_ptr<StarMesh>> meshes;
		std::vector<std::unique_ptr<StarObjectInstance>> instances; 

		std::unique_ptr<StarShaderInfo::Builder> engineBuilder;
		
		void prepareMeshes(star::core::device::DeviceContext& device); 

		virtual void createInstanceBuffers(star::core::device::DeviceContext& device, int numImagesInFlight);

		virtual void createBoundingBox(std::vector<Vertex>& verts, std::vector<uint32_t>& inds);

		// Inherited via DescriptorModifier
		virtual std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(const int& numFramesInFlight) override;
		virtual void createDescriptors(star::core::device::DeviceContext& device, const int& numFramesInFlight) override;

		virtual bool isRenderReady(core::device::DeviceContext &context); 

	private:
		static std::unique_ptr<StarDescriptorSetLayout> instanceDescriptorLayout;
		static vk::PipelineLayout extrusionPipelineLayout;
		static std::unique_ptr<Handle> tri_normalExtrusionPipeline, triAdj_normalExtrusionPipeline;
		static std::unique_ptr<StarDescriptorSetLayout> boundDescriptorLayout;
		static vk::PipelineLayout boundPipelineLayout;
		static std::unique_ptr<Handle> boundBoxPipeline;

		bool isReady = false; 

		core::device::DeviceID m_deviceID; 
		std::unique_ptr<std::vector<std::reference_wrapper<StarDescriptorSetLayout>>> groupLayout;
		std::unique_ptr<std::vector<std::vector<vk::DescriptorSet>>> globalSets;

		Handle boundingBoxVertBuffer, boundingBoxIndexBuffer;
		std::vector<std::vector<vk::DescriptorSet>> boundingDescriptors; 
		Handle vertBuffer, indBuffer;
		uint32_t boundingBoxIndsCount = 0; 

		void recordDrawCommandNormals(vk::CommandBuffer& commandBuffer);

		void recordDrawCommandBoundingBox(vk::CommandBuffer& commandBuffer, int inFlightIndex);;

		void calculateBoundingBox(std::vector<Vertex>& verts, std::vector<uint32_t>& inds);
};
}