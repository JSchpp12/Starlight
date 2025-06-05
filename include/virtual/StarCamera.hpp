#pragma once
// right hand coordinate system
// row-major notation

#include "StarEntity.hpp"

#include <glm/glm.hpp>

// Note: this camera system will fail if the user looks up the +y-axis
namespace star
{
// Camera object which will be used during rendering
class StarCamera : public StarEntity
{
  public:
    StarCamera(const uint32_t &width, const uint32_t &height); 
    StarCamera(const uint32_t &width, const uint32_t &height, const float &horizontalFieldOfView,
               const float &nearClippingPlaneDistance, const float &farClippingPlaneDistance);

    ~StarCamera() = default;

    glm::mat4 getViewMatrix() const;

    glm::mat4 getProjectionMatrix() const;

    glm::vec2 getResolution() const
    {
        return this->resolution;
    }

    float getHorizontalFieldOfView(const bool &inRadians = false) const;
    float getVerticalFieldOfView(const bool &inRadians = false) const;
    float getNearClippingDistance() const
    {
        return this->nearClippingPlaneDistance;
    }
    float getFarClippingDistance() const
    {
        return this->farClippingPlaneDistance;
    }

  protected:
    const glm::ivec2 resolution;

    float horizontalFieldOfView = 90.0f, nearClippingPlaneDistance = 0.1f, farClippingPlaneDistance = 10000.0f;
};
} // namespace star