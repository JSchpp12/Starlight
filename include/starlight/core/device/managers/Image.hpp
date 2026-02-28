#pragma once

#include "wrappers/graphics/policies/GenericImageCreateAllocatePolicy.hpp"

#include "device/managers/ManagerEventBusTies.hpp"

namespace star::core::device::manager
{
constexpr std::string_view GetImageEventTypeName = "star::core::manager::event::image";

struct ImageRequest
{
    std::optional<star::wrappers::graphics::policies::GenericImageCreateAllocatePolicy> createPolicy = std::nullopt;
    std::optional<star::StarTextures::Texture> createdTexture = std::nullopt;

    explicit ImageRequest(star::wrappers::graphics::policies::GenericImageCreateAllocatePolicy createPolicy)
        : createPolicy(std::move(createPolicy)), createdTexture(std::nullopt)
    {
    }
    explicit ImageRequest(star::StarTextures::Texture createdTexture)
        : createPolicy(std::nullopt), createdTexture(std::move(createdTexture))
    {
    }
};
struct ImageRecord
{
    star::StarTextures::Texture texture;

    bool isReady() const
    {
        return true;
    }
    void cleanupRender(device::StarDevice &device)
    {
        texture.cleanupRender(device.getVulkanDevice());
    }
};

class Image : public ManagerEventBusTies<ImageRecord, ImageRequest, 30>
{
  public:
    Image();
    Image(const Image &&) = delete;
    Image &operator=(const Image &&) = delete;
    Image(Image &&other) = default;
    Image &operator=(Image &&) = default;

    virtual ~Image() = default;

  private:
    virtual ImageRecord createRecord(ImageRequest &&request) const override;
};
} // namespace star::core::device::manager