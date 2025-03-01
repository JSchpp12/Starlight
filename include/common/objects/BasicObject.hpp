#pragma once

#include "ObjVertInfo.hpp"
#include "ObjIndicesInfo.hpp"

#include "BumpMaterial.hpp"
#include "CastHelpers.hpp"
#include "FileHelpers.hpp"
#include "StarMesh.hpp"
#include "StarDevice.hpp"
#include "StarObject.hpp"
#include "StarGraphicsPipeline.hpp"
#include "VertColorMaterial.hpp"
#include "ObjVertInfo.hpp"

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
	protected:
		std::string filePath = "";

		bool isBumpyMaterial = false; 
		bool isTextureMaterial = false; 

		Handle primaryVertBuffer, primaryIndbuffer;

		BasicObject(std::string objectFilePath) : objectFilePath(objectFilePath) {
			loadMesh();
		};

		std::string objectFilePath;
		
		void loadMesh(); 
	};
}