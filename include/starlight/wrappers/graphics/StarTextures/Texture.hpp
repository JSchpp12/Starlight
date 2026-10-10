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

namespace star::command::texture
{
struct MakeTexture;
} // namespace star::command::texture

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

    /// Compatibility facade over command::texture::MakeTexture.
    class Builder
    {
      public:
        Builder(core::device::StarDevice &device, const vk::Image &vulkanImage);

        explicit Builder(core::device::StarDevice &device);

        Builder(const Builder &other);
        Builder &operator=(const Builder &other);
        Builder(Builder &&) noexcept;
        Builder &operator=(Builder &&) noexcept;
        ~Builder();

        Builder &setCreateInfo(VmaAllocationCreateInfo nAllocInfo, vk::ImageCreateInfo nCreateInfo,
                               std::string nAllocationName);

        Builder &addViewInfo(const vk::ImageViewCreateInfo &nCreateInfo);

        Builder &setSamplerInfo(const vk::SamplerCreateInfo &nCreateInfo);

        Builder &setBaseFormat(const vk::Format &nFormat);

        Builder &setMipmapLevels(uint32_t nLevels);

        Builder &setSizeInfo(vk::DeviceSize size, vk::Extent3D resolution);

        std::unique_ptr<Texture> buildUnique();

        std::shared_ptr<Texture> buildShared();

        Texture build();

      private:
        std::unique_ptr<command::texture::MakeTexture> m_makeTexture;
    };

    Texture() = default;
    Texture(std::shared_ptr<Resources> memoryResources, const vk::Format &baseFormat, uint32_t mipmapLevels,
            const vk::Extent3D &baseExtent, vk::DeviceSize size);
    virtual ~Texture() = default;

    vk::ImageLayout &getImageLayout()
    {
        return memoryResources->trackedImageLayout;
    }
    const vk::Extent3D &getBaseExtent() const
    {
        assert(baseExtent.height != 0 && baseExtent.width != 0 && baseExtent.depth != 0);

        return baseExtent;
    }
    const vk::ImageLayout &getImageLayout() const
    {
        return memoryResources->trackedImageLayout;
    }
    void setImageLayout(vk::ImageLayout imageLayout)
    {
        memoryResources->trackedImageLayout = std::move(imageLayout);
    }
    void cleanupRender(vk::Device &device)
    {
        if (memoryResources)
        {
            memoryResources->cleanupRender(device);
        }
    }
    const vk::Image &getVulkanImage() const
    {
        return this->memoryResources->image;
    }
    const std::optional<vk::Sampler> &getSampler() const
    {
        return this->memoryResources->sampler;
    }
    vk::ImageView getImageView(const vk::Format *requestedFormat = nullptr) const;
    const vk::Format &getBaseFormat() const
    {
        return this->baseFormat;
    }
    const uint32_t &getMipmapLevels() const
    {
        return this->mipmapLevels;
    }

    vk::DeviceSize getSize() const
    {
        assert(size != 0 && "getSize() called on image that does not support size");

        return size;
    }

  protected:
    std::shared_ptr<Resources> memoryResources = std::shared_ptr<Resources>();
    vk::Format baseFormat;
    uint32_t mipmapLevels = 0;
    vk::Extent3D baseExtent = {0, 0, 0};
    vk::DeviceSize size = 0;
};
} // namespace star::StarTextures