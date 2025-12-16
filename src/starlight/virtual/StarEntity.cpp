#include "StarEntity.hpp"

#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include <iostream>

star::StarEntity::StarEntity()
    : rotationMat(glm::identity<glm::mat4>()), translationMat(glm::identity<glm::mat4>()),
      scaleMat(glm::identity<glm::mat4>())
{
}

star::StarEntity::StarEntity(const glm::vec3 &position)
    : rotationMat(glm::identity<glm::mat4>()), translationMat(glm::identity<glm::mat4>()),
      scaleMat(glm::identity<glm::mat4>())
{
    this->setPosition(position);
}

star::StarEntity::StarEntity(const glm::vec3 &position, const glm::vec3 &scale)
    : rotationMat(glm::identity<glm::mat4>()), translationMat(glm::identity<glm::mat4>()),
      scaleMat(glm::identity<glm::mat4>())
{
    this->setPosition(position);
    this->setScale(scale);
}

void star::StarEntity::setScale(const glm::vec3 &scale)
{
    if (scale.x != scale.y && scale.y != scale.z)
    {
        std::cout
            << "WARNING: Non-uniform scaling applied to object. This WILL break axis aligned bounding box calculations."
            << std::endl;
    }
    scaleMat = glm::scale(scaleMat, scale);
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