#pragma once

#include "BumpMaterial.hpp"
#include "CastHelpers.hpp"
#include "FileHelpers.hpp"
#include "StarMesh.hpp"
#include "StarDevice.hpp"
#include "StarObject.hpp"
#include "StarGraphicsPipeline.hpp"

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
		static std::unique_ptr<BasicObject> New(std::string objPath); 

		virtual ~BasicObject() = default;

		/// <summary>
		/// Build the pipeline for this object. Can use shared objects if they are provided
		/// </summary>
		/// <param name="device"></param>
		/// <param name="swapChainExtent"></param>
		/// <param name="globalDescriptorLayouts"></param>
		/// <param name="pipelineLayout"></param>
		/// <param name="renderPass"></param>
		/// <returns></returns>
		virtual std::unique_ptr<StarPipeline> buildPipeline(StarDevice& device,
			vk::Extent2D swapChainExtent, vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass) override;

		// Inherited via StarObject
		std::unordered_map<star::Shader_Stage, StarShader> getShaders() override;
	protected:
		bool isBumpyMaterial = false; 

		BasicObject(std::string objectFilePath);

		std::string objectFilePath;

		/// <summary>
		/// Load meshes from file
		/// </summary>
		/// <param name="objectFilePath">Path of the file to load</param>
		/// <returns></returns>
		void loadFromFile(const std::string objectFilePath);

	};
}