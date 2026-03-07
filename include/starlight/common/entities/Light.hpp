#pragma once

#include "Enums.hpp"
#include "StarEntity.hpp"
#include "StarObject.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <star_common/Handle.hpp>

#include <optional>

namespace star
{
class Light : public StarEntity
{
  public:
    Light() = default;

    /// <summary>
    /// Calculate the view matrix for this light source. Used when the light is being used as a shadow caster
    /// </summary>
    virtual glm::mat4 getViewMatrix() const
    {
        assert(m_direction != glm::vec3(0.0f, 0.0f, 1.0f) &&
               "The light cannot be pointed in the same direction as the arb vector in calculations.");
        glm::vec3 tmpArbVector{0.0f, 0.0f, 1.0f};
        glm::vec3 minDirection = glm::vec3(m_direction);
        auto rightVector = glm::normalize(glm::cross(tmpArbVector, minDirection));

        auto upVector = glm::normalize(glm::cross(minDirection, rightVector));

        return glm::lookAt(m_type == Type::directional ? glm::normalize(-minDirection) * glm::vec3(3.0f)
                                                       : this->positionCoords,
                           m_type == Type::directional ? minDirection : this->positionCoords + minDirection, upVector);
    }

    // Getters
    const glm::vec3 &getDirection() const
    {
        return m_direction;
    }

    const glm::vec3 &getAmbient() const
    {
        return m_ambient;
    }

    const glm::vec3 &getDiffuse() const
    {
        return m_diffuse;
    }

    const glm::vec3 &getSpecular() const
    {
        return m_specular;
    }

    float getInnerDiameter() const
    {
        return m_innerDiameter;
    }

    float getOuterDiameter() const
    {
        return m_outerDiameter;
    }

    bool isEnabled() const
    {
        return m_enabled;
    }

    Type::Light getType() const
    {
        return m_type;
    }

    bool getEnabled() const
    {
        return m_enabled;
    }

    // setters
    virtual Light &setPosition(glm::vec3 position) override
    {
        this->StarEntity::setPosition(std::move(position));
        return *this;
    }
    Light &setDirection(glm::vec3 direction)
    {
        m_direction = std::move(direction);
        return *this;
    }
    Light &setAmbient(glm::vec3 ambient)
    {
        m_ambient = std::move(ambient);
        return *this;
    }
    Light &setDiffuse(glm::vec3 diffuse)
    {
        m_diffuse = std::move(diffuse);
        return *this;
    }
    Light &setSpecular(glm::vec3 specular)
    {
        m_specular = std::move(specular);
        return *this;
    }
    Light &setInnerDiameter(float innerDiameter)
    {
        m_innerDiameter = m_innerDiameter;
        return *this;
    }
    Light &setOuterDiameter(float outerDiameter)
    {
        m_outerDiameter = m_outerDiameter;
        return *this;
    }
    Light &setLuminance(uint32_t luminance)
    {
        m_luminance = luminance;
        return *this;
    }
    Light &setEnabled(bool enabled)
    {
        m_enabled = enabled;
        return *this;
    }
    Light &setType(star::Type::Light type)
    {
        m_type = type;
        return *this;
    }

  private:
    glm::vec3 m_direction{0.0f, 1.0f, 0.0f};
    glm::vec3 m_ambient{0.5f, 0.5f, 0.5f};
    glm::vec3 m_diffuse{0.5f, 0.5f, 0.5f};
    glm::vec3 m_specular{0.5f, 0.5f, 0.5f};
    float m_innerDiameter{0.0f};
    float m_outerDiameter{1.0f};
    uint32_t m_luminance{1};
    Type::Light m_type = Type::Light::point;
    bool m_enabled = true;
};
} // namespace star