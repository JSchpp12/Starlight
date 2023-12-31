#pragma once

#include "StarTexture.hpp"

#include <memory>
#include <vector>

namespace star {
	/// <summary>
	/// Contains material info provided per mesh.
	/// </summary>
	struct StarMaterialMesh {
		std::vector<std::unique_ptr<StarTexture>> textures; 

		StarMaterialMesh(std::vector<std::unique_ptr<StarTexture>> textures)
			: textures(std::move(textures)) {}; 
	};
}