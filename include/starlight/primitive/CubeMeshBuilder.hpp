#pragma once

#include "starlight/primitive/CubeDesc.hpp"
#include "starlight/primitive/MeshData.hpp"

namespace star::primitive
{
MeshData BuildCubeMesh(const std::vector<CubeDesc> &desc, std::shared_ptr<star::StarMaterial> material);
}