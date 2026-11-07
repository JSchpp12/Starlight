#pragma once

#include "FormatInfo.hpp"
#include "StarTextures/Resources.hpp"
#include "StarTextures/TextureView.hpp"
#include "device/StarDevice.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace star::StarTextures
{
class Texture
{
  public:
    static void TransitionImageLayout(Texture &image, vk::CommandBuffer &commandBuffer, const vk::Format &format,
                                      const vk::ImageLayout &oldLayout, const vk::ImageLayout &newLayout);

    static float SelectAnisotropyLevel(const vk::PhysicalDeviceProperties &deviceProperties);

    static vk::Filter SelectTextureFiltering(const vk::PhysicalDeviceProperties &deviceProperties);

    struct Creators
    {
        Creators() = default;
        Creators(vk::ImageCreateInfo createInfo, VmaAllocationCreateInfo allocationCreateInfo,
                 std::string allocationName)
            : createInfo(std::move(createInfo)), allocationCreateInfo(std::move(allocationCreateInfo)),
              allocationName(std::move(allocationName))
        {
        }
        vk::ImageCreateInfo createInfo = vk::ImageCreateInfo();
        VmaAllocationCreateInfo allocationCreateInfo = VmaAllocationCreateInfo();
        std::string allocationName = "Default Texture Name";
    };

    Texture() = default;
    Texture(std::shared_ptr<Resources> memoryResources, const vk::Format &baseFormat, uint32_t mipmapLevels,
            const vk::Extent3D &baseExtent);
    virtual ~Texture() = default;

    vk::ImageLayout &getImageLayout()
    {
        static vk::ImageLayout blankLayout = vk::ImageLayout::eUndefined;
        return memoryResources ? memoryResources->trackedImageLayout : blankLayout;
    }
    const vk::Extent3D &getBaseExtent() const
    {
        // A blank texture (failed transfer) has no resources and a zero extent; only sanity-check real textures.
        if (memoryResources)
            assert(baseExtent.height != 0 && baseExtent.width != 0 && baseExtent.depth != 0);

        return baseExtent;
    }
    const vk::ImageLayout &getImageLayout() const
    {
        static const vk::ImageLayout blankLayout = vk::ImageLayout::eUndefined;
        return memoryResources ? memoryResources->trackedImageLayout : blankLayout;
    }
    void setImageLayout(vk::ImageLayout imageLayout)
    {
        if (memoryResources)
            memoryResources->trackedImageLayout = std::move(imageLayout);
    }
    void cleanupRender(vk::Device &device)
    {
        if (memoryResources)
        {
            memoryResources->cleanupRender(device);
        }
    }
    /// @brief The image, or a null handle when this is a blank texture (for example a failed transfer).
    const vk::Image &getVulkanImage() const
    {
        static const vk::Image blankImage{};
        return memoryResources ? memoryResources->image : blankImage;
    }
    const std::optional<vk::Sampler> &getSampler() const
    {
        static const std::optional<vk::Sampler> blankSampler{};
        return memoryResources ? memoryResources->sampler : blankSampler;
    }
    /// @brief All views created for this texture, keyed by {format, view type}.
    /// @details A single image can expose several views (e.g. a cube view for sampling and a 2D-array view for
    /// storage). Query them with TextureView::get(format) for the sampling view, or
    /// TextureView::get(format, viewType) for an exact pair. A blank texture carries no views, so every lookup
    /// returns a null handle.
    const TextureView &getImageView() const
    {
        static const TextureView blankViews{};
        return memoryResources ? memoryResources->views : blankViews;
    }
    const vk::Format &getBaseFormat() const
    {
        return this->baseFormat;
    }
    const uint32_t &getMipmapLevels() const
    {
        return this->mipmapLevels;
    }

  protected:
    std::shared_ptr<Resources> memoryResources = std::shared_ptr<Resources>();
    vk::Format baseFormat;
    uint32_t mipmapLevels = 0;
    vk::Extent3D baseExtent = {0, 0, 0};
};
} // namespace star::StarTextures
