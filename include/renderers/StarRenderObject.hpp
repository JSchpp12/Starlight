#pragma once 

#include "Handle.hpp"
#include "GameObject.hpp"
#include "Vertex.hpp"
#include "Mesh.hpp"
#include "StarTexture.hpp"
#include "StarDevice.hpp"
#include "StarRenderMesh.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <vector>
#include <string>

namespace star {
class StarRenderObject {
public:
	//eventually use for adding new buffers and shader data to the object which will be queried before draw time
	class Builder {
	public:
		Builder(StarDevice& starDevice, GameObject& gameObject);
		Builder& addMesh(std::unique_ptr<StarRenderMesh> renderMesh);

		/// <summary>
		/// Record the number of images in the swapchain. This will be used to resize the descriptor bindings. 
		/// </summary>
		/// <param name="numImages"></param>
		/// <returns></returns>
		Builder& setNumFrames(size_t numImages);
		std::unique_ptr<StarRenderObject> build();

	protected:


	private:
		StarDevice& starDevice;
		GameObject& gameObject;
		size_t numSwapChainImages;
		std::vector<std::unique_ptr<StarRenderMesh>> meshes;
	};

	StarRenderObject(StarDevice& starDevice, GameObject& gameObject, std::vector<std::unique_ptr<StarRenderMesh>> meshes,
		size_t numImages = 0) : starDevice(starDevice), meshes(std::move(meshes)),
		gameObject(gameObject), uboDescriptorSets(numImages) { }
	void initDescriptorLayouts(StarDescriptorSetLayout::Builder& constLayout);

	/// <summary>
	/// Init object with needed descriptors
	/// </summary>
	/// <param name="descriptorWriter"></param>
	void initDescriptors(StarDescriptorSetLayout& constLayout, StarDescriptorPool& descriptorPool);
	void render(vk::CommandBuffer& commandBuffer, vk::PipelineLayout& pipelineLayout, int swapChainIndexNum);

	void prepRender(StarDevice& device); 

	//TODO: might want to create render function for each mesh as they get more complicated
	//void render(vk::CommandBuffer& commandBuffer); 

	GameObject& getGameObject() { return this->gameObject; }
	std::vector<vk::DescriptorSet>& getDefaultDescriptorSets();
	//TODO: remove this
	const std::vector<std::unique_ptr<StarRenderMesh>>& getMeshes() { return this->meshes; }

protected:


private:
	StarDevice& starDevice;
	GameObject& gameObject;
	//TODO: I would like to make the descriptor sets a unique_ptr
	Handle objectHandle;
	std::vector<vk::DescriptorSet> uboDescriptorSets;
	std::vector<std::unique_ptr<StarRenderMesh>> meshes;

};
}