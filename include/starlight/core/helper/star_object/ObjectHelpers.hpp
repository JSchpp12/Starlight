#pragma once

#include "starlight/virtual/StarObject.hpp"

#include <glm/glm.hpp>

namespace star::core::helper::star_object
{
std::array<std::pair<star::Type::Axis, float>, 3> ConvertFromEulerToGlobalRotations(const glm::vec3 &rDeg);

glm::vec3 ExtractRotationDegrees(const glm::mat4 &objectRotationMatrix);

} // namespace star::core::helper::star_object