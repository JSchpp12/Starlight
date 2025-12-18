#pragma once
// right hand coordinate system
// row-major notation

#include "StarEntity.hpp"
#include "core/device/DeviceContext.hpp"

#include <glm/glm.hpp>

// Note: this camera system will fail if the user looks up the +y-axis
namespace star
{
// Camera object which will be used during rendering
class StarCamera : public StarEntity
{
  public:
    StarCamera() = default;
    StarCamera(const uint32_t &width, const uint32_t &height);
    StarCamera(const uint32_t &width, const uint32_t &height, const float &horizontalFieldOfView,
               const float &nearClippingPlaneDistance, const float &farClippingPlaneDistance);
    virtual ~StarCamera() = default;

    virtual void frameUpdate(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex) {};
    glm::mat4 getViewMatrix() const;

    glm::mat4 getProjectionMatrix() const;

    glm::ivec2 &getResolution()
    {
        return resolution;
    }
    const glm::ivec2 &getResolution() const
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
    glm::ivec2 resolution{0, 0};

    float horizontalFieldOfView = 90.0f, nearClippingPlaneDistance = 0.1f, farClippingPlaneDistance = 10000.0f;
};
} // namespace star