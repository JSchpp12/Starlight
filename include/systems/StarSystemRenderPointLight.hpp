#pragma once 

#include "StarShader.hpp"
#include "Light.hpp"
#include "StarSystemRenderObj.hpp"
#include "StarRenderObject.hpp"

#include <iostream>
#include <vector>
#include <memory>

namespace star {
class StarSystemRenderPointLight : private StarSystemRenderObject {
public:
	StarSystemRenderPointLight(const StarSystemRenderPointLight&) = delete;

	StarSystemRenderPointLight(StarDevice& device, size_t numSwapChainImages, vk::DescriptorSetLayout globalSetLayout,
		vk::Extent2D swapChainExtent, vk::RenderPass renderPass) :
		StarSystemRenderObject(device, numSwapChainImages, globalSetLayout, swapChainExtent, renderPass) { }

	virtual ~StarSystemRenderPointLight();

	virtual void init(std::vector<vk::DescriptorSetLayout> globalSets) override { this->StarSystemRenderObject::init(globalSets); }

	virtual void addLight(Light* newLight, std::unique_ptr<StarRenderObject> linkedRenderObject, size_t numSwapChainImages);

	virtual void registerShader(vk::ShaderStageFlagBits stage, StarShader& newShader, Handle newShaderHandle) override {
		this->StarSystemRenderObject::registerShader(stage, newShader, newShaderHandle);
	};

	virtual void bind(vk::CommandBuffer& commandBuffer) { this->StarSystemRenderObject::bind(commandBuffer); }

	virtual void updateBuffers(uint32_t currentImage) override;

	virtual void render(vk::CommandBuffer& commandBuffer, int swapChainImageIndex) { this->StarSystemRenderObject::render(commandBuffer, swapChainImageIndex); }

	virtual void setPipelineLayout(vk::PipelineLayout newPipelineLayout) { this->StarSystemRenderObject::setPipelineLayout(newPipelineLayout); }

	virtual bool hasShader(vk::ShaderStageFlagBits stage) { return this->StarSystemRenderObject::hasShader(stage); }

	virtual Handle getBaseShader(vk::ShaderStageFlags stage) { return this->StarSystemRenderObject::getBaseShader(stage); }
protected:

private:
	std::vector<Light*> lightList;

	//might be able to change this to be a template method
	virtual void createRenderBuffers() override;

	virtual void createDescriptors() override;

};
}