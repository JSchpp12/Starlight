#include "device/managers/Image.hpp"

#include <star_common/HandleTypeRegistry.hpp>

namespace star::core::device::manager
{
Image::Image() : Base(GetImageEventTypeName)
{
}

ImageRecord Image::createRecord(ImageRequest &&request) const
{
    auto *texture = std::get_if<star::StarTextures::Texture>(&request.payload);
    assert(texture != nullptr && "A pending image request must be handled by the base createSlot");
    return ImageRecord{std::move(*texture)};
}
} // namespace star::core::device::manager
