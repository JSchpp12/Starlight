#pragma once

#include "StarObject.hpp"

#include <tiny_obj_loader.h>

#include <memory>
#include <string>
#include <vector>

namespace star {
	/// <summary>
	/// Basic object for use with rendering. This object is loaded from an .obj file 
	/// and is attached to a simple shader with textures and a graphics pipeline for 
	/// those shader types.
	/// </summary>
	class BasicObject : public StarObject {
	public:
		static std::unique_ptr<BasicObject> New(const std::string objPath); 

		virtual ~BasicObject() = default;

		virtual std::unordered_map<star::Shader_Stage, StarShader> getShaders() override;

		virtual void prepRender(star::core::device::DeviceContext& context, vk::Extent2D swapChainExtent,
			vk::PipelineLayout pipelineLayout, core::renderer::RenderingTargetInfo renderingInfo, int numSwapChainImages, 
			StarShaderInfo::Builder fullEngineBuilder) override; 

		virtual void prepRender(star::core::device::DeviceContext& context, int numSwapChainImages, 
			Handle sharedPipeline, star::StarShaderInfo::Builder fullEngineBuilder) override;
	protected:
		std::string filePath = "";

		bool isBumpyMaterial = false; 
		bool isTextureMaterial = false; 

		Handle primaryVertBuffer, primaryIndbuffer;

		BasicObject(std::string objectFilePath) : objectFilePath(objectFilePath){}

		std::string objectFilePath;
		
		void loadMesh(core::device::DeviceContext &context); 
	};
}