#pragma once

#include "StarDevice.hpp"
#include "StarTextures/Resources.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

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

    class Builder
    {
      public:
        Builder(vk::Device &device, const vk::Image &vulkanImage);

        Builder(vk::Device &device, VmaAllocator &allocator);

        Builder &setCreateInfo(const VmaAllocationCreateInfo &nAllocInfo, const vk::ImageCreateInfo &nCreateInfo,
                               const std::string &nAllocationName);

        Builder &addViewInfo(const vk::ImageViewCreateInfo &nCreateInfo);

        Builder &setSamplerInfo(const vk::SamplerCreateInfo &nCreateInfo);

        Builder &setBaseFormat(const vk::Format &nFormat);

        std::unique_ptr<Texture> build();

      private:
        struct Creators
        {
            Creators(VmaAllocator &allocator) : allocator(allocator)
            {
            }

            VmaAllocator &allocator;
            vk::ImageCreateInfo createInfo = vk::ImageCreateInfo();
            VmaAllocationCreateInfo allocationCreateInfo = VmaAllocationCreateInfo();
            std::string allocationName = "Default Texture Name";
        };

        vk::Device &device;
        std::optional<vk::Format> format = std::nullopt;
        std::optional<Creators> createNewAllocationInfo = std::nullopt;
        std::optional<vk::Image> vulkanImage = std::nullopt;

        std::vector<vk::ImageViewCreateInfo> viewInfos = std::vector<vk::ImageViewCreateInfo>();
        std::optional<vk::SamplerCreateInfo> samplerInfo = std::nullopt;
    };

    virtual ~Texture();

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

  protected:
    Texture(vk::Device &device, vk::Image &vulkanImage, const std::vector<vk::ImageViewCreateInfo> &imageViewInfos,
            const vk::Format &baseFormat);

    Texture(vk::Device &device, vk::Image &vulkanImage, const std::vector<vk::ImageViewCreateInfo> &imageViewInfos,
            const vk::Format &baseFormat, const vk::SamplerCreateInfo &samplerInfo);

    Texture(vk::Device &device, VmaAllocator &allocator, const vk::ImageCreateInfo &createInfo,
            const std::string &allocationName, const VmaAllocationCreateInfo &allocationCreateInfo,
            const std::vector<vk::ImageViewCreateInfo> &imageViewInfos, const vk::Format &baseFormat);

    Texture(vk::Device &device, VmaAllocator &allocator, const vk::ImageCreateInfo &createInfo,
            const std::string &allocationName, const VmaAllocationCreateInfo &allocationCreateInfo,
            const std::vector<vk::ImageViewCreateInfo> &imageViewInfos, const vk::Format &baseFormat,
            const vk::SamplerCreateInfo &samplerInfo);

    std::shared_ptr<Resources> memoryResources = std::shared_ptr<Resources>();

    vk::Device &device;
    const vk::Format baseFormat = vk::Format();

    const uint32_t mipmapLevels = 1;

    static std::shared_ptr<StarTextures::Resources> CreateResource(vk::Device &device, const vk::Format &baseFormat,
                                                                   VmaAllocator &allocator,
                                                                   const VmaAllocationCreateInfo &allocationCreateInfo,
                                                                   const vk::ImageCreateInfo &imageCreateInfo,
                                                                   const std::string &allocationName);

    static std::shared_ptr<StarTextures::Resources> CreateResource(
        vk::Device &device, const vk::Format &baseFormat, VmaAllocator &allocator,
        const VmaAllocationCreateInfo &allocationCreateInfo, const vk::ImageCreateInfo &imageCreateInfo,
        const std::string &allocationName, const std::vector<vk::ImageViewCreateInfo> &imageViewCreateInfos);

    static std::shared_ptr<StarTextures::Resources> CreateResource(
        vk::Device &device, const vk::Format &baseFormat, VmaAllocator &allocator,
        const VmaAllocationCreateInfo &allocationCreateInfo, const vk::ImageCreateInfo &imageCreateInfo,
        const std::string &allocationName, const std::vector<vk::ImageViewCreateInfo> &imageViewCreateInfos,
        const vk::SamplerCreateInfo &samplerCreateInfo);

    static vk::Sampler CreateImageSampler(vk::Device &device, const vk::SamplerCreateInfo &samplerCreateInfo);

    static void CreateAllocation(vk::Device &device, const vk::Format &baseFormat, VmaAllocator &allocator,
                                 const VmaAllocationCreateInfo &allocationCreateInfo,
                                 const vk::ImageCreateInfo &imageCreateInfo, VmaAllocation &allocation,
                                 vk::Image &textureImage, const std::string &allocationName);

    static vk::ImageView CreateImageView(vk::Device &device, const vk::ImageViewCreateInfo &imageViewCreateInfo);

    static uint32_t ExtractMipmapLevels(const vk::ImageViewCreateInfo &imageViewInfo);

    static std::unordered_map<vk::Format, vk::ImageView> CreateImageViews(
        vk::Device &device, vk::Image vulkanImage, std::vector<vk::ImageViewCreateInfo> imageViewCreateInfo);

    static void VerifyImageCreateInfo(const vk::ImageCreateInfo &createInfo);

    friend class Builder;
};
} // namespace star::StarTextures