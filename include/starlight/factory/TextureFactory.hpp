#pragma once

#include "StarTextures/Resources.hpp"
#include "StarTextures/Texture.hpp"
#include "StarTextures/TextureView.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace star::StarTextures
{

/// @brief Builds the underlying Vulkan objects for a texture.
class TextureFactory
{
  public:
    /// @brief Create the image + VMA allocation for a new texture (no views or sampler), verifying hardware support.
    static std::shared_ptr<Resources> CreateImage(core::device::StarDevice &device, const Texture::Creators &creators,
                                                  const vk::Format &baseFormat);

    /// @brief Low-level image + VMA allocation creation (no hardware verification).
    static std::shared_ptr<Resources> CreateResource(core::device::StarDevice &device, const vk::Format &baseFormat,
                                                     VmaAllocator &allocator,
                                                     const VmaAllocationCreateInfo &allocationCreateInfo,
                                                     const vk::ImageCreateInfo &imageCreateInfo,
                                                     const std::string &allocationName);

    /// @brief Dedicated builder for a texture's image views from ordered view create infos.
    static TextureView CreateViews(core::device::StarDevice &device, vk::Image image,
                                   std::span<const vk::ImageViewCreateInfo> viewInfos, uint32_t mipmapLevels,
                                   const std::string &debugName);

    static vk::Sampler CreateImageSampler(core::device::StarDevice &device,
                                          const vk::SamplerCreateInfo &samplerCreateInfo,
                                          const std::string &debugName = {});

    /// @brief Build a complete Texture for a newly allocated image once its resources (views/sampler) are attached.
    static Texture FinishNewAllocation(const Texture::Creators &creators, const vk::Format &baseFormat,
                                       uint32_t mipmapLevels, std::shared_ptr<Resources> resources);

    /// @brief Build a complete Texture for a newly allocated image: creates and names the image/allocation, builds
    ///        and names the views, and assembles the final Texture. All debug names derive from
    ///        creators.allocationName.
    static Texture CreateAllocatedTexture(core::device::StarDevice &device, const Texture::Creators &creators,
                                          const vk::Format &baseFormat,
                                          std::span<const vk::ImageViewCreateInfo> viewInfos, uint32_t mipmapLevels);

    /// @brief CreateAllocatedTexture with an additional image sampler, named from creators.allocationName.
    static Texture CreateAllocatedTexture(core::device::StarDevice &device, const Texture::Creators &creators,
                                          const vk::Format &baseFormat,
                                          std::span<const vk::ImageViewCreateInfo> viewInfos, uint32_t mipmapLevels,
                                          const vk::SamplerCreateInfo &samplerInfo);

    /// @brief Build a complete Texture for an externally owned Vulkan image: names the image, builds and names the
    ///        views, and assembles the final Texture. All debug names derive from debugName.
    static Texture CreateTexture(core::device::StarDevice &device, const vk::Image &vulkanImage,
                                 const vk::Format &baseFormat, std::span<const vk::ImageViewCreateInfo> viewInfos,
                                 uint32_t mipmapLevels, const std::string &debugName, const vk::Extent3D &resolution,
                                 vk::DeviceSize size);

    /// @brief CreateTexture with an additional image sampler, named from debugName.
    static Texture CreateTexture(core::device::StarDevice &device, const vk::Image &vulkanImage,
                                 const vk::Format &baseFormat, std::span<const vk::ImageViewCreateInfo> viewInfos,
                                 uint32_t mipmapLevels, const vk::SamplerCreateInfo &samplerInfo,
                                 const std::string &debugName, const vk::Extent3D &resolution, vk::DeviceSize size);

    static vk::DeviceSize CalculateSize(const vk::Format &baseFormat, const vk::Extent3D &baseExtent,
                                        const uint32_t &arrayLayers, const vk::ImageType &imageType,
                                        const uint32_t &mipmapLevels);

  private:
    static void VerifyNewAllocation(core::device::StarDevice &device, const Texture::Creators &creators,
                                    const vk::Format &baseFormat);

    static void CreateAllocation(core::device::StarDevice &device, const vk::Format &baseFormat,
                                 VmaAllocator &allocator, const VmaAllocationCreateInfo &allocationCreateInfo,
                                 const vk::ImageCreateInfo &imageCreateInfo, VmaAllocation &allocation,
                                 vk::Image &textureImage, const std::string &allocationName);

    static void VerifyImageCreateInfo(const vk::ImageCreateInfo &createInfo);

    static void LogImageCreateFailure(const vk::Result &result);

    static vk::Extent3D GetLevelExtent(const vk::Extent3D &baseExtent, const uint32_t &level);

    static uint32_t CeilDiv(const uint32_t &width, const uint32_t &height);
};
} // namespace star::StarTextures
