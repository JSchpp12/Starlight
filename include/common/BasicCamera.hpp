#pragma once
#include "Interactivity.hpp"
#include "KeyStates.hpp"
#include "StarCamera.hpp"
#include "Time.hpp"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>

namespace star
{
/// <summary>
/// Camera with default controls
/// </summary>
class BasicCamera : public StarCamera, public Interactivity
{
  public:
    BasicCamera(const uint32_t &width, const uint32_t &height);
    BasicCamera(const uint32_t &width, const uint32_t &height, const float &horizontalFieldOfView,
                const float &nearClippingPlaneDistance, const float &farClippingPlaneDistance,
                const float &movementSpeed, const float &sensitivity);

    /// <summary>
    /// Key callback for camera object. Implements default controls for the camera.
    /// </summary>
    /// <param name="key"></param>
    /// <param name="scancode"></param>
    /// <param name="action"></param>
    /// <param name="mods"></param>
    virtual void onKeyPress(int key, int scancode, int mods) override;

    virtual void onKeyRelease(int key, int scancode, int mods) override;

    /// <summary>
    /// Mouse callback for camera objects. Implements default controls for the camera.
    /// </summary>
    /// <param name="xpos"></param>
    /// <param name="ypos"></param>
    virtual void onMouseMovement(double xpos, double ypos) override;

    /// <summary>
    /// Mouse button callback for camera object.
    /// </summary>
    /// <param name="button"></param>
    /// <param name="action"></param>
    /// <param name="mods"></param>
    virtual void onMouseButtonAction(int button, int action, int mods) override;

    /// <summary>
    /// Update camera locations as needed
    /// </summary>
    virtual void onWorldUpdate(const uint32_t &frameInFlightIndex) override;

    void onScroll(double xoffset, double yoffset) override {};

    float getPitch() const
    {
        return this->pitch;
    }

    float getYaw() const
    {
        return this->yaw;
    }

  private:
    Time time = Time();

    const float movementSpeed = 1000.0f;
    const float sensitivity = 0.1f;
    bool init = false;

    // previous mouse coordinates from GLFW
    float prevX, prevY, xMovement, yMovement;

    // control information for camera
    float pitch = -0.f, yaw = -90.0f;

    bool click = false;
};
} // namespace star