#pragma once 

#include "StarObject.hpp"
#include "Handle.hpp"
#include "Enums.hpp"
#include "StarShader.hpp"
#include "Light.hpp"
#include "VulkanVertex.hpp"
#include "StarDescriptors.hpp"
#include "StarDevice.hpp"
#include "StarPipeline.hpp"
#include "StarBuffer.hpp"
#include "StarObject.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <vector>
#include <map>

namespace star {
	/// <summary>
	/// Base object which contains the per shader object data for vulkan. 
	/// </summary>
	class StarSystemRenderObject {
	public:
		/// <summary>
		/// Object type to be used per render object, updated each frame
		/// </summary>
		struct UniformBufferObject {
			glm::mat4 modelMatrix;
			glm::mat4 normalMatrix;
		};
		uint32_t totalNumVerticies = 0;

		StarSystemRenderObject(StarDevice& device, size_t numSwapChainImages, vk::DescriptorSetLayout globalSetLayout,
			vk::Extent2D swapChainExtent, vk::RenderPass renderPass) :
			starDevice(device), numSwapChainImages(numSwapChainImages),
			globalSetLayout(globalSetLayout), swapChainExtent(swapChainExtent),
			renderPass(renderPass) {};

		StarSystemRenderObject(const StarSystemRenderObject& baseObject);

		virtual ~StarSystemRenderObject();

		virtual void init(StarDevice& device, std::vector<vk::DescriptorSetLayout> globalDescriptorSets);

		virtual void registerShader(vk::ShaderStageFlagBits stage, StarShader& newShader, Handle newShaderHandle);

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
		/// Check if the object has a shader for the requestd stage
		/// </summary>
		/// <param name="stage"></param>
		/// <returns></returns>
		virtual bool hasShader(vk::ShaderStageFlagBits stage);

		virtual Handle getBaseShader(vk::ShaderStageFlags stage);

		virtual void setPipelineLayout(vk::PipelineLayout newPipelineLayout);

		virtual size_t getNumRenderObjects();

		virtual StarObject& getRenderObjectAt(size_t index);

		virtual void bind(vk::CommandBuffer& commandBuffer);

		virtual void updateBuffers(uint32_t currentImage);

		//void createPipeline(PipelineConfigSettings& settings); 

		/// <summary>
		/// Render the object
		/// </summary>
		virtual void render(vk::CommandBuffer& commandBuffer, int swapChainImageIndex);

		//TODO: remove
		virtual vk::PipelineLayout getPipelineLayout() { return this->pipelineLayout; }
		virtual StarDescriptorSetLayout* getSetLayout() { return this->descriptorSetLayout.get(); }
		virtual StarDescriptorPool* getDescriptorPool() { return this->descriptorPool.get(); }
		virtual StarBuffer* getBufferAt(int i) { return this->uniformBuffers.at(i).get(); }
		uint32_t getNumVerticies() { return this->totalNumVerticies; }
	protected:
		bool ownerOfSetLayout = true;
		StarDevice& starDevice;
		int numSwapChainImages = 0;
		int numMeshes = 0;

		vk::Extent2D swapChainExtent;
		vk::RenderPass renderPass;

		std::unique_ptr<StarBuffer> vertexBuffer;
		std::unique_ptr<StarBuffer> indexBuffer;

		Handle vertShaderHandle;
		StarShader* vertShader = nullptr;
		Handle fragShaderHandle;
		StarShader* fragShader = nullptr;

		vk::DescriptorSetLayout globalSetLayout;
		std::vector<Light*> lights;
		std::vector<std::reference_wrapper<StarObject>> renderObjects;
		std::unique_ptr<StarDescriptorPool> descriptorPool;
		std::unique_ptr<StarPipeline> starPipeline;
		vk::PipelineLayout pipelineLayout;
		std::vector<std::unique_ptr<StarBuffer>> uniformBuffers;

		std::unique_ptr<StarDescriptorSetLayout> staticDescriptorSetLayout;		//descriptor set layout for object data updated once
		std::unique_ptr<StarDescriptorSetLayout> descriptorSetLayout;

		std::vector<std::vector<vk::DescriptorSet>> staticDescriptorSets;		//descriptor sets for object data updated on init
		std::vector<std::vector<vk::DescriptorSet>> descriptorSets;

		/// <summary>
		/// Concat all verticies of all objects into a single buffer and copy to gpu.
		/// </summary>
		virtual void createVertexBuffer();

		virtual void createIndexBuffer();

		virtual void createDescriptorPool();
		/// <summary>
		/// Create buffers needed for render operations. Such as those used by descriptors
		/// </summary>
		virtual void createRenderBuffers();
		virtual void createDescriptorLayouts();
		/// <summary>
		/// Create descriptors for binding render buffers to shaders.
		/// </summary>
		virtual void createDescriptors();

		virtual void createPipelineLayout(std::vector<vk::DescriptorSetLayout> globalDescriptorSets);

		virtual void createPipeline();

	private:

	};
}