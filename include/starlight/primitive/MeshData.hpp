#pragma once

#include "StarMaterial.hpp"
#include "Vertex.hpp"

#include <memory>
#include <vector>

namespace star::primitive
{
struct MeshData
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::shared_ptr<StarMaterial> material;
};
} // namespace star::primitive
