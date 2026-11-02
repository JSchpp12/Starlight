#include "starlight/primitive/cube/TextureColor.hpp"

namespace star::primitive::cube
{
TextureColor::TextureColor(star::Handle textureHandle) : m_textureHandle(textureHandle)
{
}

star::Handle TextureColor::getHandle() const
{
    return m_textureHandle;
}

} // namespace star::primitive::cube
