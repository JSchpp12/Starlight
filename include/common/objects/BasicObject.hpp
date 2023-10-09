#pragma once

#include "FileHelpers.hpp"
#include "StarMesh.hpp"
#include "StarDevice.hpp"
#include "StarObject.hpp"

#include <tiny_obj_loader.h>

#include <memory>
#include <string>
#include <vector>

namespace star {
	class BasicObject : public StarObject {
	public:
		static std::unique_ptr<BasicObject> New(std::string objPath); 

		virtual ~BasicObject() = default; 

		std::vector<std::unique_ptr<StarMesh>> loadMeshes() override;
	protected:
		BasicObject(std::string objectFilePath)
			: StarObject(), objectFilePath(objectFilePath){};

		std::string objectFilePath;

		/// <summary>
		/// Load meshes from file
		/// </summary>
		/// <param name="objectFilePath">Path of the file to load</param>
		/// <returns></returns>
		static std::vector<std::unique_ptr<StarMesh>> loadFromFile(const std::string objectFilePath);
	};
}