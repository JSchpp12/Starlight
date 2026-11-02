#pragma once

#include <star_common/Handle.hpp>

namespace star::primitive::cube
{
/// @brief Colors a cube using a texture.
class TextureColor
{
  public:
    TextureColor() = default;
    explicit TextureColor(star::Handle textureHandle);

    [[nodiscard]] star::Handle getHandle() const;

  private:
    star::Handle m_textureHandle;
};

} // namespace star::primitive::cube
