#pragma once 

#include "CastHelpers.hpp"
#include "StarObject.hpp"
#include "Handle.hpp"
#include "Enums.hpp"
#include "StarShader.hpp"
#include "Light.hpp"
#include "VulkanVertex.hpp"
#include "StarDescriptorBuilders.hpp"
#include "StarDevice.hpp"
#include "StarGraphicsPipeline.hpp"
#include "StarObject.hpp"
#include "StarShaderInfo.hpp"
#include "StarCommandBuffer.hpp"
#include "DescriptorModifier.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <vector>
#include <map>

namespace star {
	/// <summary>
	/// Class which contains a group of objects which share pipeline dependencies
	/// which are compatible with one another. Implies that they can all share 
	/// same pipeline layout but NOT the same pipeline.
	/// </summary>
	class StarRenderGroup : private DescriptorModifier {
	public:

		StarRenderGroup(StarDevice& device, size_t numSwapChainImages,
			vk::Extent2D swapChainExtent, StarObject& baseObject); 

		//no copy
		StarRenderGroup(const StarRenderGroup& baseObject) = delete;

		virtual ~StarRenderGroup();

		virtual void init(StarShaderInfo::Builder initEngineBuilder, RenderingTargetInfo renderingInfo);

		virtual bool isObjectCompatible(StarObject& object); 

		/// <summary>
		/// Add a new rendering object which will be rendered with the pipeline contained in this vulkan object.
		/// </summary>
		/// <param name="newObjectHandle"></param>
		//virtual void addObject(Handle newObjectHandle, GameObject* newObject, size_t numSwapChainImages);

		/// <summary>
		/// Register a light color and location for rendering light effects on objects
		/// </summary>
		virtual void addObject(StarObject& newRenderObject);

		virtual void addLight(Light* newLight) { this->lights.push_back(newLight); }

		/// <summary>
		/// Render the object
		/// </summary>
		virtual void recordRenderPassCommands(vk::CommandBuffer& mainDrawBuffer, const int &swapChainImageIndex);

		virtual void recordPreRenderPassCommands(vk::CommandBuffer& mainDrawBuffer, const int &swapChainImageIndex); 

		virtual void recordPostRenderPassCommands(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex); 

		//TODO: remove
		virtual vk::PipelineLayout getPipelineLayout() { return this->pipelineLayout; }
		int getNumObjects() { return numObjects; }

	protected:
		struct RenderObjectInfo {
			StarObject& object;
			uint32_t startVBIndex, startIBIndex; 

			RenderObjectInfo(StarObject& object)
				: object(object){};
		};

		/// <summary>
		/// Groups of objects that can share a pipeline -- they have the same shader
		/// </summary>
		struct Group {
			RenderObjectInfo baseObject;
			std::vector<RenderObjectInfo> objects;

			Group(RenderObjectInfo baseObject) : baseObject(baseObject) {}
		};

		StarDevice& device;
		int numSwapChainImages = 0;
		int numObjects = 0; 
		int numMeshes = 0; 
		vk::Extent2D swapChainExtent;

		std::unique_ptr<StarPipeline> starPipeline; 
		vk::PipelineLayout pipelineLayout;
		std::vector<std::shared_ptr<StarDescriptorSetLayout>> largestDescriptorSet;

		std::vector<Light*> lights;

		std::vector<Group> groups;

		/// <summary>
		/// Create descriptors for binding render buffers to shaders.
		/// </summary>
		virtual void prepareObjects(StarShaderInfo::Builder& groupBuilder, RenderingTargetInfo renderingInfo);

		virtual void createPipelineLayout(std::vector<std::shared_ptr<StarDescriptorSetLayout>>& fullSetLayout);

		// Inherited via DescriptorModifier
		std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(const int& numFramesInFlight) override;
		void createDescriptors(star::StarDevice& device, const int& numFramesInFlight) override;
	};
}