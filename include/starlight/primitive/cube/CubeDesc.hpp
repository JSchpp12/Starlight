#pragma once

#include "starlight/primitive/cube/SolidColor.hpp"
#include "starlight/primitive/cube/TextureColor.hpp"

#include <variant>

namespace star::primitive::cube
{
/// @brief Description of one cube instance in a CubeObject.
struct CubeDesc
{
    std::variant<SolidColor, TextureColor> coloringApproach{SolidColor{}};
};

} // namespace star::primitive::cube
