#pragma once

#include "Enums.hpp"

#include <glm/glm.hpp>

namespace star
{
class StarEntity
{
  public:
    StarEntity();
    StarEntity(const glm::vec3 &position);
    StarEntity(const glm::vec3 &position, const glm::vec3 &scale);
    virtual ~StarEntity() = default;

    glm::vec3 getPosition() const
    {
        glm::mat4 matrixCopy = getDisplayMatrix();
        return glm::vec3{matrixCopy[3][0], matrixCopy[3][1], matrixCopy[3][2]};
    }

    glm::vec3 getScale() const;

    virtual void setScale(const glm::vec3 &scale);

    virtual void setPosition(const glm::vec3 &newPosition);

    /// <summary>
    /// Apply translation to object's current position vector and update accordingly
    /// </summary>
    /// <param name="movement"></param>
    virtual void moveRelative(const glm::vec3 &movement);

    /// <summary>
    /// Move entity in a direction defined by a normalized direction vector.
    /// </summary>
    /// <param name="movementDirection">Movement direction vector. Will be normalized.</param>
    /// <param name="movementAmt">Ammount to move the entity by</param>
    virtual void moveRelative(const glm::vec3 &movementDirection, const float &movementAmt);

    /// <summary>
    /// Rotate object about a relative axis
    /// </summary>
    /// <param name="amt">Ammount of rotation to apply</param>
    /// <param name="rotationVector">Vector around which to apply rotation</param>
    /// <param name="inDegrees">Is the amount provided in degrees</param>
    void rotateRelative(const Type::Axis &axis, const float &amt, const bool &inDegrees = true);

    void rotateGlobal(const Type::Axis &axis, const float &amt, const bool &inDegrees = true);

    void setForwardVector(const glm::vec3 &newForward)
    {
        glm::vec3 f = glm::normalize(newForward);
        // RH, Y-up world up
        const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

        // Protect against parallel forward/up
        glm::vec3 u = worldUp;
        if (std::abs(glm::dot(f, worldUp)) > 0.9995f)
        {
            u = glm::vec3(0.0f, 0.0f, 1.0f); // fallback up if nearly parallel
        }

        // RH orthonormal frame:
        glm::vec3 r = glm::normalize(glm::cross(u, f)); // right = up × forward
        u = glm::normalize(glm::cross(f, r));           // recompute exact up

        forwardVector = glm::vec4(f, 0.0f);
        upVector = glm::vec4(u, 0.0f);
        rightVector = glm::vec4(r, 0.0f);
    }

    const glm::vec4 &getForwardVector() const
    {
        return this->forwardVector;
    }
    const glm::vec4 &getUpVector() const
    {
        return this->upVector;
    }
    virtual glm::mat4 getDisplayMatrix() const
    {
        return translationMat * rotationMat * scaleMat;
    }
    const glm::mat4 &getRotationMat() const
    {
        return rotationMat;
    }

  protected:
    glm::vec3 positionCoords = glm::vec3();
    glm::mat4 rotationMat = glm::mat4(1.0f);
    glm::mat4 translationMat = glm::mat4(1.0f);
    glm::mat4 scaleMat = glm::mat4(1.0f);

  private:
    glm::vec4 rightVector = glm::vec4{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec4 upVector = glm::vec4{0.0f, 1.0f, 0.0f, 0.0f};     // original up vector of object
    glm::vec4 forwardVector = glm::vec4{0.0f, 0.0, 1.0f, 0.0f}; // original forward vector of object

    void updateCoordsRot(const glm::mat4 &rotMat)
    {
        this->upVector = glm::normalize(rotMat * this->upVector);
        this->forwardVector = glm::normalize(rotMat * this->forwardVector);

        const auto r = glm::normalize(glm::cross(glm::vec3(upVector), glm::vec3(forwardVector)));
        this->rightVector = glm::vec4(r, 0.0);
    }
};
} // namespace star