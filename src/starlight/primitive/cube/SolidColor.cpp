#include "starlight/primitive/cube/SolidColor.hpp"

namespace star::primitive::cube
{
SolidColor::SolidColor(const star::Color &color) : m_color(color)
{
}

const star::Color &SolidColor::getColor() const
{
    return m_color;
}

} // namespace star::primitive::cube
