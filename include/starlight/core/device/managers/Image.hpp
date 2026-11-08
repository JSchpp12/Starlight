#pragma once

#include "wrappers/graphics/policies/GenericImageCreateAllocatePolicy.hpp"

#include "device/managers/ManagerEventBusTies.hpp"

namespace star::core::device::manager
{
constexpr std::string_view GetImageEventTypeName = "stImgE";

/// @brief Compile-time name of the image resource type, used for handle registration and failure logging.
struct ImageResourceTypeName
{
    static constexpr std::string_view GetUniqueTypeName()
    {
        return common::special_types::GetImageTypeName;
    }
};

struct ImageRequest : public ManagerResourceRequest<star::StarTextures::Texture>
{
    using Base = ManagerResourceRequest<star::StarTextures::Texture>;
    using Base::Base;

    explicit ImageRequest(star::wrappers::graphics::policies::GenericImageCreateAllocatePolicy createPolicy)
        : Base(createPolicy.create())
    {
    }
};

struct ImageRecord
{
    star::StarTextures::Texture texture;

    star::StarTextures::Texture *getTexture()
    {
        return &texture;
    }
    const star::StarTextures::Texture *getTexture() const
    {
        return &texture;
    }

    void cleanupRender(device::StarDevice &device)
    {
        texture.cleanupRender(device.getVulkanDevice());
    }
};

class Image : public ManagerEventBusTies<ImageRecord, ImageRequest, 30, ImageResourceTypeName>
{
  public:
    using Base = ManagerEventBusTies<ImageRecord, ImageRequest, 30, ImageResourceTypeName>;

    Image();
    Image(const Image &&) = delete;
    Image &operator=(const Image &&) = delete;
    Image(Image &&other) = default;
    Image &operator=(Image &&other) = default;

    virtual ~Image() = default;

    using Base::get;

  private:
    virtual ImageRecord createRecord(ImageRequest &&request) const override;
};
} // namespace star::core::device::manager
