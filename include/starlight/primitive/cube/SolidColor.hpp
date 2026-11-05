#pragma once

#include "starlight/structs/Color.hpp"

namespace star::primitive::cube
{
/// @brief Colors a cube using a single solid color.
class SolidColor
{
  public:
    SolidColor() = default;
    explicit SolidColor(const star::Color &color);

    [[nodiscard]] const star::Color &getColor() const;

  private:
    star::Color m_color;
};

} // namespace star::primitive::cube
