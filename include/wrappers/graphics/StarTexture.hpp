#pragma once

#include "StarDevice.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>


#include <optional>
#include <unordered_map>
#include <vector>

namespace star
{
class StarTexture
{
  public:
    static void TransitionImageLayout(StarTexture &image, vk::CommandBuffer &commandBuffer, const vk::Format &format, const vk::ImageLayout &oldLayout, const vk::ImageLayout &newLayout); 
    
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

        Builder& setBaseFormat(const vk::Format& nFormat); 

        std::unique_ptr<StarTexture> build();

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

    ~StarTexture();

    const vk::Image &getVulkanImage() const
    {
        return this->vulkanImage;
    }
    const std::optional<vk::Sampler> &getSampler() const
    {
        return this->sampler;
    }
    vk::ImageView getImageView(const vk::Format *requestedFormat = nullptr) const;
    const vk::Format &getBaseFormat() const{
        return this->baseFormat;
    }
    const uint32_t &getMipmapLevels() const{
        return this->mipmapLevels;
    }
  protected:
    StarTexture(vk::Device& device, vk::Image &vulkanImage, const std::vector<vk::ImageViewCreateInfo> &imageViewInfos, 
                const vk::Format& baseFormat);

    StarTexture(vk::Device& device, vk::Image &vulkanImage, const std::vector<vk::ImageViewCreateInfo> &imageViewInfos,
                const vk::Format& baseFormat,
                const vk::SamplerCreateInfo &samplerInfo);

    StarTexture(vk::Device& device, VmaAllocator &allocator, const vk::ImageCreateInfo &createInfo,
                const std::string &allocationName, const VmaAllocationCreateInfo &allocationCreateInfo,
                const std::vector<vk::ImageViewCreateInfo> &imageViewInfos, const vk::Format& baseFormat);

    StarTexture(vk::Device& device, VmaAllocator &allocator, const vk::ImageCreateInfo &createInfo,
                const std::string &allocationName, const VmaAllocationCreateInfo &allocationCreateInfo,
                const std::vector<vk::ImageViewCreateInfo> &imageViewInfos, const vk::Format& baseFormat,
                const vk::SamplerCreateInfo &samplerInfo);

    vk::Device &device;
    vk::Image vulkanImage = vk::Image();
    std::optional<VmaAllocator *> allocator = nullptr;
    const vk::Format baseFormat;

    std::unordered_map<vk::Format, vk::ImageView> views = std::unordered_map<vk::Format, vk::ImageView>();
    const uint32_t mipmapLevels = 1;
    std::optional<VmaAllocation> memory = std::nullopt;
    std::optional<vk::Sampler> sampler = std::nullopt;


    static vk::Sampler CreateImageSampler(vk::Device &device, const vk::SamplerCreateInfo &samplerCreateInfo);

    static void CreateAllocation(vk::Device &device, const vk::Format& baseFormat, VmaAllocator &allocator,
                                 const VmaAllocationCreateInfo &allocationCreateInfo,
                                 const vk::ImageCreateInfo &imageCreateInfo, VmaAllocation &allocation,
                                 vk::Image &textureImage, const std::string &allocationName);

    static vk::ImageView CreateImageView(vk::Device &device, const vk::ImageViewCreateInfo &imageCreateInfo);

    static uint32_t ExtractMipmapLevels(const vk::ImageViewCreateInfo& imageViewInfo); 

    static std::unordered_map<vk::Format, vk::ImageView> CreateImageViews(
        vk::Device &device, vk::Image vulkanImage, std::vector<vk::ImageViewCreateInfo> imageCreateInfos);

    static void VerifyImageCreateInfo(const vk::ImageCreateInfo& createInfo); 

    friend class Builder;
};
} // namespace star