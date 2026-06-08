#pragma once

#include "starlight/structs/Color.hpp"

#include <glm/glm.hpp>

namespace star::primitive
{

struct CubeDesc
{
    glm::vec3 size = glm::vec3(1.0f);
    star::Color color;
};

} // namespace star::primitive