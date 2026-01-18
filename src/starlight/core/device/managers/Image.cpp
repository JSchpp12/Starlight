#include "device/managers/Image.hpp"

#include <star_common/HandleTypeRegistry.hpp>

namespace star::core::device::manager
{
Image::Image()
    : ManagerEventBusTies<ImageRecord, ImageRequest, 30>(common::special_types::GetImageTypeName, GetImageEventTypeName)
{
}

ImageRecord Image::createRecord(ImageRequest &&request) const
{
    if (request.createPolicy.has_value())
    {
        return {request.createPolicy.value().create()};
    }

    assert(request.createdTexture.has_value());
    return {request.createdTexture.value()};
}
} // namespace star::core::device::manager