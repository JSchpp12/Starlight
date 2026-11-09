#pragma once

#include "Vertex.hpp"

#include <vector>

namespace star::primitive
{
struct MeshData
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};
} // namespace star::primitive
