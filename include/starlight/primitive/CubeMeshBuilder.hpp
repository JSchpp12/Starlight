#pragma once

#include "starlight/primitive/CubeDesc.hpp"
#include "starlight/primitive/MeshData.hpp"

namespace star::primitive
{
MeshData BuildCubeMesh(const std::vector<CubeDesc> &desc, std::shared_ptr<star::StarMaterial> material);

/// Unit cube wireframe: 8 corner vertices and 24 indices forming the 12 cube edges as a line list.
MeshData BuildCubeWireMesh();
} // namespace star::primitive