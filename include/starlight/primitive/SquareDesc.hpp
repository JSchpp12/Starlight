#pragma once

#include <glm/glm.hpp>

namespace star::primitive
{
struct SquareDesc
{
    glm::vec2 size = {1.0f, 1.0f};
    glm::vec4 color = {1.0f, 0.0f, 1.0f, 1.0f};
};
} // namespace star::primitive
