#include "StarCamera.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

star::StarCamera::StarCamera(const uint32_t &width, const uint32_t &height) : resolution(glm::ivec2{width, height})
{
}
star::StarCamera::StarCamera(const uint32_t &width, const uint32_t &height, const float &horizontalFieldOfView,
                             const float &nearClippingPlaneDistance, const float &farClippingPlaneDistance)
    : resolution(glm::ivec2{width, height}), horizontalFieldOfView(horizontalFieldOfView),
      nearClippingPlaneDistance(nearClippingPlaneDistance), farClippingPlaneDistance(farClippingPlaneDistance)
{
}

glm::mat4 star::StarCamera::getViewMatrix() const
{
    return glm::lookAt(this->getPosition(), this->getPosition() + glm::vec3(this->getForwardVector()),
                       glm::vec3(this->getUpVector()));
}

glm::mat4 star::StarCamera::getProjectionMatrix() const
{
    auto projection =
        glm::perspective(getVerticalFieldOfView(true), (float)this->resolution.x / (float)this->resolution.y,
                         this->nearClippingPlaneDistance, this->farClippingPlaneDistance);

    // vulkan is flipped along y axis
    projection[1][1] *= -1;
    return projection;
}

float star::StarCamera::getHorizontalFieldOfView(const bool &inRadians) const
{
    if (inRadians)
        return glm::radians(this->horizontalFieldOfView);
    else
        return this->horizontalFieldOfView;
}

float star::StarCamera::getVerticalFieldOfView(const bool &inRadians) const
{
    const float verticalRadians = glm::radians(this->horizontalFieldOfView);
    const float resultRadians =
        2.0f * glm::atan(glm::tan(verticalRadians / 2.0f) / (float)(this->resolution.x / (float)this->resolution.y));

    if (inRadians)
        return resultRadians;
    else
        return glm::degrees(resultRadians);
}