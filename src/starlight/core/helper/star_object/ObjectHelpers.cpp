#include "starlight/core/helper/star_object/ObjectHelpers.hpp"

namespace star::core::helper::star_object
{
std::array<std::pair<star::Type::Axis, float>, 3> ConvertFromEulerToGlobalRotations(const glm::vec3 &rDeg)
{
    float ax = rDeg[0];
    float ay = rDeg[1];
    float az = rDeg[2];
    return std::array<std::pair<star::Type::Axis, float>, 3>{
        std::make_pair(star::Type::Axis::z, az),
        std::make_pair(star::Type::Axis::y, ay),
        std::make_pair(star::Type::Axis::x, ax),
    };
}

glm::vec3 ExtractRotationDegrees(const glm::mat4 &r)
{
    float sy = -r[0][2];
    float cy = sqrtf(1.0f - sy * sy);

    float x, y, z;

    if (cy > 1e-6f)
    {
        // Standard Y-up Euler XYZ decomposition
        x = atan2f(r[1][2], r[2][2]); // rotX
        y = asinf(sy);                // rotY
        z = atan2f(r[0][1], r[0][0]); // rotZ
    }
    else
    {
        // Gimbal lock case
        x = atan2f(-r[1][0], r[1][1]);
        y = asinf(sy);
        z = 0.0f;
    }

    return glm::degrees(glm::vec3(x, y, z));
}
} // namespace star::core::helper::star_object
