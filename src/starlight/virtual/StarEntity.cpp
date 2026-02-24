#include "StarEntity.hpp"

#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include <cassert>

star::StarEntity::StarEntity()
    : rotationMat(glm::mat4(1.0f)), translationMat(glm::mat4(1.0f)), scaleMat(glm::mat4(1.0f))
{
}

star::StarEntity::StarEntity(const glm::vec3 &position)
    : rotationMat(glm::mat4(1.0f)), translationMat(glm::mat4(1.0f)), scaleMat(glm::mat4(1.0f))
{
    this->setPosition(position);
}

star::StarEntity::StarEntity(const glm::vec3 &position, const glm::vec3 &scale)
    : rotationMat(glm::mat4(1.0f)), translationMat(glm::mat4(1.0f)), scaleMat(glm::mat4(1.0f))
{
    this->setPosition(position);
    this->setScale(scale);
}

void star::StarEntity::setScale(const glm::vec3 &scale)
{
    scaleMat = glm::scale(glm::mat4(1.0f), scale);
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
}

void star::StarEntity::moveRelative(const glm::vec3 &movement)
{
    // need to update model matrix before applying further translations
    this->translationMat = glm::translate(translationMat, movement);
    this->positionCoords = translationMat * glm::vec4(positionCoords, 1.0);
}

void star::StarEntity::moveRelative(const glm::vec3 &movementDirection, const float &movementAmt)
{
    auto normMove = glm::normalize(movementDirection);
    glm::vec3 movement = glm::vec3{normMove.x * movementAmt, normMove.y * movementAmt, normMove.z * movementAmt};

    this->translationMat = glm::translate(translationMat, movement);
}

static inline void RefreshBasisFromMatrix(glm::mat4 &m, glm::vec3 &right, glm::vec3 &up, glm::vec3 &forward)
{
    right = glm::normalize(glm::vec3(m[0]));
    up = glm::normalize(glm::vec3(m[1]));
    forward = glm::normalize(glm::cross(right, up));

    m[0] = glm::vec4(right, 0.0f);
    m[1] = glm::vec4(up, 0.0f);
    m[2] = glm::vec4(forward, 0.0f);
}

void star::StarEntity::rotateRelative(star::Type::Axis axis, const float &amt, bool inDegrees)
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
        rotationVector = glm::vec3{1.0, 0.0, 0.0};
    }
    else if (axis == Type::Axis::y)
    {
        rotationVector = glm::vec3{0.0, 1.0, 0.0};
    }
    else if (axis == Type::Axis::z)
    {
        rotationVector = glm::vec3{0.0, 0.0, 1.0};
    }
    // might want to normalize vector
    rotationVector = glm::normalize(rotationVector);
    const auto rDelta = glm::rotate(glm::mat4(1.0), radians, rotationVector);
    rotationMat = rotationMat * rDelta;
}

void star::StarEntity::setForwardVector(const glm::vec3 &newForward)
{
    glm::vec3 f = glm::normalize(newForward);
    // RH, Y-up world up
    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

    // Protect against parallel forward/up
    glm::vec3 u = worldUp;
    if (std::abs(glm::dot(f, worldUp)) > 0.9995f)
    {
        u = glm::vec3(0.0f, 1.0f, 0.0f); // fallback up if nearly parallel
    }

    // RH orthonormal frame:
    glm::vec3 r = glm::normalize(glm::cross(f, u)); // right = up × forward
    u = glm::normalize(glm::cross(r, f));           // recompute exact up
    rotationMat[0] = glm::vec4(r, 0.0f);
    rotationMat[1] = glm::vec4(u, 0.0f);
    rotationMat[2] = glm::vec4(f, 0.0f);
}

void star::StarEntity::rotateGlobal(Type::Axis axis, const float &amt, bool inDegrees)
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

    const glm::mat4 R = glm::rotate(glm::mat4(1.0f), radians, glm::normalize(rotationVector));
    rotationMat = rotationMat * R; // WORLD rotation: pre-multiply
}