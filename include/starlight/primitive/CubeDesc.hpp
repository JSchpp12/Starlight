#pragma once

#include "starlight/structs/Color.hpp"

#include <glm/glm.hpp>

namespace star::primitive
{
struct CubeDesc
{
    glm::vec3 size{1.0, 1.0, 1.0};
    star::Color color;
};

} // namespace star::primitive