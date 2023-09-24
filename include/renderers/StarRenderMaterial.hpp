/*
* This class will contain all information needed to render a material. The Material class only contains handles
* to required members (i.e. textures). These required members will be collected from managers into this class for rendering
* operations.
*/
#pragma once 
#include "Enums.hpp"
#include "Material.hpp"
#include "Texture.hpp"
#include "StarDevice.hpp"
#include "StarTexture.hpp"
#include "StarDescriptors.hpp"
#include "MaterialManager.hpp"
#include "TextureManager.hpp"
#include "MapManager.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>

namespace star {
class StarRenderMaterial {
public:
	class Builder {
	public:
		Builder(StarDevice& starDevice, MaterialManager& materialManager, TextureManager& textureManager, MapManager& mapManager)
			: starDevice(starDevice), materialManager(materialManager),
			textureManager(textureManager), mapManager(mapManager) { }
		Builder& setMaterial(Handle materialHandle);
		std::unique_ptr<StarRenderMaterial> build();

	private:
		StarDevice& starDevice;
		MaterialManager& materialManager;
		TextureManager& textureManager;
		MapManager& mapManager;
		Material* material = nullptr;
	};
	/// <summary>
	/// Initalize the const descriptor set layouts with needed descriptor slots
	/// </summary>
	/// <param name="constBuilder"></param>
	void initDescriptorLayouts(StarDescriptorSetLayout::Builder& constBuilder);
	/// <summary>
	/// Init Render Material with proper descriptors
	/// </summary>
	/// <param name="staticDescriptorSetLayout">DescriptorSetLayout to be used when creating object descriptors which are updated once (during init)</param>
	void buildConstDescriptor(StarDescriptorWriter writer);

	//need to gather refs to base materials
	std::unique_ptr<StarTexture> texture;
	std::unique_ptr<StarTexture> bumpMap;

	StarRenderMaterial(StarDevice& starDevice, Material& material, Texture& texture, Texture& bumpMap);

	void bind(vk::CommandBuffer& commandBuffer, vk::PipelineLayout pipelineLayout, int swapChainImageIndex);

	//todo: might want to move all descriptor creation to individual classes
	//void addRequiredBindings();

	//get 
	Material& getMaterial() { return this->material; }
	StarTexture& getTexture() { return *this->texture; }
	vk::DescriptorSet& getDescriptor() { return this->descriptor; }

private:
	StarDevice& starDevice;
	Material& material;
	vk::DescriptorSet descriptor;

};
}