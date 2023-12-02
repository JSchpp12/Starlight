#pragma once 

#include "CastHelpers.hpp"
#include "StarObject.hpp"
#include "Handle.hpp"
#include "Enums.hpp"
#include "StarShader.hpp"
#include "Light.hpp"
#include "VulkanVertex.hpp"
#include "StarDescriptors.hpp"
#include "StarDevice.hpp"
#include "StarGraphicsPipeline.hpp"
#include "StarBuffer.hpp"
#include "StarObject.hpp"
#include "StarSystemPipeline.hpp"
#include "StarCommandBuffer.hpp"

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
	class StarRenderGroup  {
	public:

		StarRenderGroup(StarDevice& device, size_t numSwapChainImages,
			vk::Extent2D swapChainExtent, StarObject& baseObject, 
			uint32_t baseObj_VBStart, uint32_t baseObj_IBStart); 

		StarRenderGroup(const StarRenderGroup& baseObject) = default;

		virtual ~StarRenderGroup();

		virtual void init(StarDescriptorSetLayout& engineSetLayout, vk::RenderPass engineRenderPass,
			 std::vector<vk::DescriptorSet> enginePerImageDescriptors);

		virtual bool isObjectCompatible(StarObject& object); 

		/// <summary>
		/// Add a new rendering object which will be rendered with the pipeline contained in this vulkan object.
		/// </summary>
		/// <param name="newObjectHandle"></param>
		//virtual void addObject(Handle newObjectHandle, GameObject* newObject, size_t numSwapChainImages);

		/// <summary>
		/// Register a light color and location for rendering light effects on objects
		/// </summary>
		virtual void addObject(StarObject& newRenderObject, uint32_t indexStartOffset, uint32_t vertexStartOffset);

		virtual void addLight(Light* newLight) { this->lights.push_back(newLight); }

		/// <summary>
		/// Render the object
		/// </summary>
		virtual void recordRenderPassCommands(StarCommandBuffer& mainDrawBuffer, int swapChainImageIndex);

		virtual void recordPreRenderPassCommands(StarCommandBuffer& mainDrawBuffer, int swapChainImageIndex); 

		//TODO: remove
		virtual vk::PipelineLayout getPipelineLayout() { return this->pipelineLayout; }
		int getNumObjects() { return numObjects; }

	protected:
		struct RenderObjectInfo {
			StarObject& object;
			uint32_t startVBIndex, startIBIndex; 

			RenderObjectInfo(StarObject& object,uint32_t startVBIndex, uint32_t startIBIndex)
				: object(object), startVBIndex(startVBIndex), startIBIndex(startIBIndex) {};
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
		std::vector<std::unique_ptr<StarDescriptorSetLayout>> largestDescriptorSet;

		std::vector<Light*> lights;

		std::vector<Group> groups;

		/// <summary>
		/// Create descriptors for binding render buffers to shaders.
		/// </summary>
		virtual void prepareObjects(StarDescriptorSetLayout& engineLayout, vk::RenderPass engineRenderPass,
			std::vector<vk::DescriptorSet> enginePerImageDescriptors);

		std::vector<std::vector<vk::DescriptorSet>> generateObjectExternalDescriptors(int objectOffset,
			std::vector<vk::DescriptorSet> enginePerObjectDescriptors);

		virtual void createPipelineLayout(StarDescriptorSetLayout& engineSetLayout);
	};
}