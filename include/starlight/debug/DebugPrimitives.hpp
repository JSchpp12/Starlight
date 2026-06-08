#pragma once

#include "starlight/object/StarObject.hpp"
#include "starlight/primitive/CubeDesc.hpp"

#include <glm/glm.hpp>
#include <memory>

namespace star::debug
{
struct SquareCreateInfo
{
    glm::vec2 size{1.0f, 1.0f};
    glm::vec4 color{1.0f, 0.0f, 1.0f, 1.0f};

    bool visible = true;
    bool drawNormals = false;
    bool drawBoundingBox = false;
};

std::unique_ptr<StarObject> CreateSquare(const SquareCreateInfo &info = {});
std::shared_ptr<StarObject> CreateCube(std::vector<star::primitive::CubeDesc> info);

} // namespace star::debug
