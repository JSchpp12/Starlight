#include "StarEntity.hpp"

#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include <cassert>

star::StarEntity::StarEntity()
    : rotationMat(glm::mat4(1.0f)), translationMat(glm::mat4(1.0f)),
      scaleMat(glm::mat4(1.0f))
{
}

star::StarEntity::StarEntity(const glm::vec3 &position)
    : rotationMat(glm::mat4(1.0f)), translationMat(glm::mat4(1.0f)),
      scaleMat(glm::mat4(1.0f))
{
    this->setPosition(position);
}

star::StarEntity::StarEntity(const glm::vec3 &position, const glm::vec3 &scale)
    : rotationMat(glm::mat4(1.0f)), translationMat(glm::mat4(1.0f)),
      scaleMat(glm::mat4(1.0f))
{
    this->setPosition(position);
    this->setScale(scale);
}

void star::StarEntity::setScale(const glm::vec3 &scale)
{
    scaleMat = glm::scale(scaleMat, scale);
}

glm::vec3 star::StarEntity::getScale() const
{
    glm::mat4 RS = rotationMat * scaleMat; // exclude translation
    glm::vec3 col0 = glm::vec3(RS[0]);     // X basis (affected by scale)
    glm::vec3 col1 = glm::vec3(RS[1]);     // Y basis
    glm::vec3 col2 = glm::vec3(RS[2]);     // Z basis
    return glm::vec3(glm::length(col0), glm::length(col1), glm::length(col2));
}

void star::StarEntity::setPosition(const glm::vec3 &newPosition)
{
    this->positionCoords = newPosition;
    this->translationMat = glm::translate(newPosition);
    this->updateCoordsTranslation(translationMat);
}

void star::StarEntity::moveRelative(const glm::vec3 &movement)
{
    // need to update model matrix before applying further translations
    this->translationMat = glm::translate(translationMat, movement);
    this->positionCoords = translationMat * glm::vec4(positionCoords, 1.0);
    this->updateCoordsTranslation(translationMat);
}

void star::StarEntity::moveRelative(const glm::vec3 &movementDirection, const float &movementAmt)
{
    auto normMove = glm::normalize(movementDirection);
    glm::vec3 movement = glm::vec3{normMove.x * movementAmt, normMove.y * movementAmt, normMove.z * movementAmt};

    this->translationMat = glm::translate(translationMat, movement);
    this->updateCoordsTranslation(translationMat);
}

void star::StarEntity::rotateRelative(const star::Type::Axis &axis, const float &amt, const bool &inDegrees)
{
    float radians = 0.0f;
    glm::vec3 rotationVector = glm::vec3();
    assert((axis == Type::Axis::x || axis == Type::Axis::y || axis == Type::Axis::z) && "Invalid axis type provided");

    if (inDegrees)
    {
        radians = glm::radians(amt);
    }
    else
    {
        radians = amt;
    }

    if (axis == Type::Axis::x)
    {
        rotationVector = xAxis;
    }
    else if (axis == Type::Axis::y)
    {
        rotationVector = yAxis;
    }
    else if (axis == Type::Axis::z)
    {
        rotationVector = zAxis;
    }
    // might want to normalize vector
    rotationVector = glm::normalize(rotationVector);

    rotationMat = glm::rotate(rotationMat, radians, rotationVector);

    this->updateCoordsRot(rotationMat);
}

void star::StarEntity::rotateGlobal(const Type::Axis &axis, const float &amt, const bool &inDegrees)
{
    float radians = 0.0f;
    glm::vec3 rotationVector = glm::vec3();
    assert((axis == Type::Axis::x || axis == Type::Axis::y || axis == Type::Axis::z) && "Invalid axis type provided");

    if (inDegrees)
    {
        radians = glm::radians(amt);
    }
    else
    {
        radians = amt;
    }

    if (axis == Type::Axis::x)
    {
        rotationVector = glm::vec3{1.0f, 0.0f, 0.0f};
    }
    else if (axis == Type::Axis::y)
    {
        rotationVector = glm::vec3{0.0f, 1.0f, 0.0f};
    }
    else if (axis == Type::Axis::z)
    {
        rotationVector = glm::vec3{0.0f, 0.0f, 1.0f};
    }

    rotationVector = glm::normalize(rotationVector);
    rotationMat = glm::rotate(rotationMat, radians, rotationVector);

    this->updateCoordsRot(rotationMat);
}