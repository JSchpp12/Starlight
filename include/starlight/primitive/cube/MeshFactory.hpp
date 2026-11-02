#pragma once

#include "starlight/primitive/MeshData.hpp"
#include "starlight/primitive/cube/CubeDesc.hpp"

namespace star::primitive::cube
{
MeshData BuildCubeMesh();

/// Unit cube wireframe: 8 corner vertices and 24 indices forming the 12 cube edges as a line list.
MeshData BuildCubeWireMesh();
} // namespace star::primitive::cube