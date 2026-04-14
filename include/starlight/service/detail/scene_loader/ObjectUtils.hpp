#pragma once

#include "starlight/virtual/StarObject.hpp"

#include <array>
#include <glm/glm.hpp>

namespace star::service::scene_loader
{
std::array<std::pair<star::Type::Axis, float>, 3> ConvertFromEulerToGlobalRotations(const glm::vec3 &rDeg);

glm::vec3 ExtractRotationDegrees(const StarObject &object);

}