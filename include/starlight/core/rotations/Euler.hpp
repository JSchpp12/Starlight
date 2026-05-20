#pragma once

#include <glm/glm.hpp>

namespace star::core::rotation
{
class Euler
{
    float m_data[3]{0.0f, 0.0f, 0.0f};

  public:
    Euler(float x, float y, float z) : m_data { std::move(x), std::move(y), std::move(z) };
    Euler(const glm::mat4 &rotationMatrix); 

};
}