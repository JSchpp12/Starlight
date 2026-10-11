#pragma once

#include "StarTextures/Resources.hpp"
#include "StarTextures/Texture.hpp"
#include "StarTextures/TextureView.hpp"
#include "StarTextures/TextureViewBuilder.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace star::StarTextures
{

/// @brief Builds the underlying Vulkan objects for a texture.
class TextureFactory
{
  public:
    static std::shared_ptr<Resources> CreateResource(vk::Device &device, const vk::Format &baseFormat,
                                                     VmaAllocator &allocator,
                                                     const VmaAllocationCreateInfo &allocationCreateInfo,
                                                     const vk::ImageCreateInfo &imageCreateInfo,
                                                     const std::string &allocationName);

    static std::shared_ptr<Resources> CreateResource(vk::Device &device, const vk::Format &baseFormat,
                                                     VmaAllocator &allocator,
                                                     const VmaAllocationCreateInfo &allocationCreateInfo,
                                                     const vk::ImageCreateInfo &imageCreateInfo,
                                                     const std::string &allocationName,
                                                     const TextureViewBuilder &viewInfos, uint32_t mipmapLevels);

    static std::shared_ptr<Resources> CreateResource(vk::Device &device, const vk::Format &baseFormat,
                                                     VmaAllocator &allocator,
                                                     const VmaAllocationCreateInfo &allocationCreateInfo,
                                                     const vk::ImageCreateInfo &imageCreateInfo,
                                                     const std::string &allocationName,
                                                     const TextureViewBuilder &viewInfos, uint32_t mipmapLevels,
                                                     const vk::SamplerCreateInfo &samplerCreateInfo);

    /// @brief Build a complete Texture for a newly allocated image.
    /// @param device
    /// @param creators
    /// @param baseFormat
    /// @param viewInfos
    /// @param mipmapLevels
    /// @return
    static Texture CreateTexture(core::device::StarDevice &device, const Texture::Creators &creators,
                                 const vk::Format &baseFormat, const TextureViewBuilder &viewInfos,
                                 uint32_t mipmapLevels);

    static Texture CreateTexture(core::device::StarDevice &device, const Texture::Creators &creators,
                                 const vk::Format &baseFormat, const TextureViewBuilder &viewInfos,
                                 uint32_t mipmapLevels, const vk::SamplerCreateInfo &samplerInfo);

    /// @brief Build a complete Texture for an externally owned Vulkan image.
    /// @param device
    /// @param vulkanImage
    /// @param baseFormat
    /// @param viewInfos
    /// @param mipmapLevels
    /// @param resolution
    /// @param size
    /// @return
    static Texture CreateTexture(core::device::StarDevice &device, const vk::Image &vulkanImage,
                                 const vk::Format &baseFormat, const TextureViewBuilder &viewInfos,
                                 uint32_t mipmapLevels, const vk::Extent3D &resolution, vk::DeviceSize size);

    static Texture CreateTexture(core::device::StarDevice &device, const vk::Image &vulkanImage,
                                 const vk::Format &baseFormat, const TextureViewBuilder &viewInfos,
                                 uint32_t mipmapLevels, const vk::SamplerCreateInfo &samplerInfo,
                                 const vk::Extent3D &resolution, vk::DeviceSize size);

    static vk::Sampler CreateImageSampler(vk::Device &device, const vk::SamplerCreateInfo &samplerCreateInfo);

    static vk::DeviceSize CalculateSize(const vk::Format &baseFormat, const vk::Extent3D &baseExtent,
                                        const uint32_t &arrayLayers, const vk::ImageType &imageType,
                                        const uint32_t &mipmapLevels);

  private:
    static void VerifyNewAllocation(core::device::StarDevice &device, const Texture::Creators &creators,
                                    const vk::Format &baseFormat);

    static Texture FinishNewAllocation(const Texture::Creators &creators, const vk::Format &baseFormat,
                                       uint32_t mipmapLevels, std::shared_ptr<Resources> resources);

    static void CreateAllocation(vk::Device &device, const vk::Format &baseFormat, VmaAllocator &allocator,
                                 const VmaAllocationCreateInfo &allocationCreateInfo,
                                 const vk::ImageCreateInfo &imageCreateInfo, VmaAllocation &allocation,
                                 vk::Image &textureImage, const std::string &allocationName);

    static void VerifyImageCreateInfo(const vk::ImageCreateInfo &createInfo);

    static void LogImageCreateFailure(const vk::Result &result);

    static vk::Extent3D GetLevelExtent(const vk::Extent3D &baseExtent, const uint32_t &level);

    static uint32_t CeilDiv(const uint32_t &width, const uint32_t &height);
};
} // namespace star::StarTextures
