#pragma once

#include "Enums.hpp"
#include <starlight/common/Handle.hpp>
#include "StarEntity.hpp"
#include "StarObject.hpp"

#include <glm/glm.hpp>

#include <optional>

namespace star
{
class Light : public StarEntity
{
  public:
    Type::Light type = Type::Light::point;
    glm::vec4 direction = glm::vec4{0.0f, 1.0f, 0.0f, 0.0f};
    glm::vec4 ambient = glm::vec4{0.5f, 0.5f, 0.5f, 1.0f};
    glm::vec4 diffuse = glm::vec4{0.5f, 0.5f, 0.5f, 1.0f};
    glm::vec4 specular = glm::vec4{0.5f, 0.5f, 0.5f, 1.0f};
    Light() = default;
    Light(const glm::vec3 &position, const Type::Light &type) : StarEntity(position), type(type)
    {
    }

    Light(const glm::vec3 &position, const Type::Light &type, const glm::vec3 &direction)
        : StarEntity(position), type(type), direction(glm::vec4(direction, 1.0))
    {
    }

    virtual ~Light() = default;

    /// <summary>
    /// Turn the light off or on. Default behavior is to simply switch the light status.
    /// </summary>
    /// <param name="state">the state to set the light to. If none is provided, default state is used</param>
    virtual void setEnabled(const bool *state = nullptr)
    {
        if (state != nullptr)
        {
            enabled = *state;
        }
        else
        {
            // flip value
            enabled = !enabled;
        }
    }

    virtual void setInnerDiameter(const float &amt)
    {
        // inner diameter must always be less than outer diameter and greater than 0
        if (amt < outerDiameter && amt > 0)
        {
            innerDiameter = amt;
        }
    }

    virtual void setOuterDiameter(const float &amt)
    {
        // outer diamater must always be greater than inner diameter
        if (amt > innerDiameter)
        {
            outerDiameter = amt;
        }
    }

    /// <summary>
    /// Calculate the view matrix for this light source. Used when the light is being used as a shadow caster
    /// </summary>
    virtual glm::mat4 getViewMatrix()
    {
        assert(direction != glm::vec4(0.0f, 1.0f, 0.0f, 0.0f) &&
               "The light cannot be pointed in the same direction as the arb vector in calculations.");
        glm::vec3 tmpArbVector{0.0f, 0.0f, 1.0f};
        glm::vec3 minDirection = glm::vec3(this->direction);
        auto rightVector = glm::normalize(glm::cross(tmpArbVector, minDirection));

        auto upVector = glm::normalize(glm::cross(minDirection, rightVector));

        return glm::lookAt(type == Type::directional ? glm::normalize(-minDirection) * glm::vec3(3.0f)
                                                     : this->positionCoords,
                           type == Type::directional ? minDirection : this->positionCoords + minDirection, upVector);
    }

    bool getEnabled() const
    {
        return enabled;
    }
    Type::Light getType() const
    {
        return type;
    }
    glm::vec4 getAmbient() const
    {
        return ambient;
    }
    glm::vec4 getDiffuse() const
    {
        return diffuse;
    }
    glm::vec4 getSpecular() const
    {
        return specular;
    }
    float getInnerDiameter() const
    {
        return innerDiameter;
    }
    float getOuterDiameter() const
    {
        return outerDiameter;
    }

  private:
    float innerDiameter = 0.0f;
    float outerDiameter = 1.0f;
    bool enabled = true;
};
} // namespace star