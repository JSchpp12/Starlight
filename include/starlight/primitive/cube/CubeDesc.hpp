#pragma once

#include "starlight/primitive/cube/SolidColor.hpp"
#include "starlight/primitive/cube/TextureColor.hpp"

#include <glm/glm.hpp>

#include <variant>

namespace star::primitive::cube
{
struct CubeDesc
{
    glm::vec3 size{1.0, 1.0, 1.0};
    std::variant<SolidColor, TextureColor> coloringApproach{SolidColor{}};
};

} // namespace star::primitive::cube
