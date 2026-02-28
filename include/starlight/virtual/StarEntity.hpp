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
    void rotateRelative(Type::Axis axis, const float &amt, bool inDegrees = true);

    void rotateGlobal(Type::Axis axis, const float &amt, bool inDegrees = true);

    void setForwardVector(const glm::vec3 &newForward);

    const glm::vec4 &getForwardVector() const
    {
        return this->rotationMat[2];
    }
    const glm::vec4 &getUpVector() const
    {
        return this->rotationMat[1];
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
};
} // namespace star