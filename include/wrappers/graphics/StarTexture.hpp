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
    static float SelectAnisotropyLevel(const vk::PhysicalDeviceProperties &deviceProperties);

    static vk::Filter SelectTextureFiltering(const vk::PhysicalDeviceProperties &deviceProperties);

    class Builder
    {
      public:
        Builder(StarDevice &device, const vk::Image &vulkanImage);

        Builder(StarDevice &device, VmaAllocator &allocator);

        Builder &setCreateInfo(const VmaAllocationCreateInfo &nAllocInfo, const vk::ImageCreateInfo &nCreateInfo,
                               const std::string &nAllocationName);

        Builder &setViewInfo(const vk::ImageViewCreateInfo &nCreateInfo);

        Builder &setSamplerInfo(const vk::SamplerCreateInfo &nCreateInfo);

        Builder &setExtraInfo(const std::string &nAllocationName);

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

        StarDevice &device;
        std::optional<Creators> createNewAllocationInfo = std::nullopt;
        std::optional<vk::Image> vulkanImage = std::nullopt;

        std::vector<vk::ImageViewCreateInfo> viewInfos = std::vector<vk::ImageViewCreateInfo>();
        std::optional<vk::SamplerCreateInfo> samplerInfo = std::nullopt;
    };

    virtual ~StarTexture();

    void transitionLayout(vk::CommandBuffer &commandBuffer, vk::ImageLayout newLayout, vk::AccessFlags srcFlags,
                          vk::AccessFlags dstFlags, vk::PipelineStageFlags sourceStage,
                          vk::PipelineStageFlags dstStage);

    const vk::Image &getVulkanImage() const
    {
        return this->vulkanImage;
    }
    const vk::Sampler &getSampler() const
    {
        return this->sampler;
    }
    const vk::ImageView &getImageView(const vk::Format *requestedFormat = nullptr) const;

  protected:
    StarTexture(StarDevice &device, vk::Image &vulkanImage, const std::vector<vk::ImageViewCreateInfo> &imageViewInfos);

    StarTexture(StarDevice &device, vk::Image &vulkanImage, const std::vector<vk::ImageViewCreateInfo> &imageViewInfos,
                const vk::SamplerCreateInfo &samplerInfo);

    StarTexture(StarDevice &device, VmaAllocator &allocator, const vk::ImageCreateInfo &createInfo,
                const std::string &allocationName, const VmaAllocationCreateInfo &allocationCreateInfo,
                const std::vector<vk::ImageViewCreateInfo> &imageViewInfos);

    StarTexture(StarDevice &device, VmaAllocator &allocator, const vk::ImageCreateInfo &createInfo,
                const std::string &allocationName, const VmaAllocationCreateInfo &allocationCreateInfo,
                const std::vector<vk::ImageViewCreateInfo> &imageViewInfos, const vk::SamplerCreateInfo &samplerInfo);

    StarDevice &device;
    std::optional<VmaAllocator *> allocator = nullptr;
    std::optional<VmaAllocation> memory = std::nullopt;

    vk::Image vulkanImage = vk::Image();
    vk::Sampler sampler = vk::Sampler();
    std::unordered_map<vk::Format, vk::ImageView> views = std::unordered_map<vk::Format, vk::ImageView>();

    void extractImageInfo();

    void createTextureImageView(const vk::Format &viewFormat, const vk::ImageAspectFlags &aspectFlags);

    static vk::ImageView CreateImageView(vk::Device &device, vk::Image image, vk::Format format,
                                         const vk::ImageAspectFlags &aspectFlags, const int &mipMapLevels);

    static vk::Sampler CreateImageSampler(vk::Device &device, const vk::SamplerCreateInfo &samplerCreateInfo);

    static void CreateAllocation(vk::Device &device, VmaAllocator &allocator,
                                 const VmaAllocationCreateInfo &allocationCreateInfo,
                                 const vk::ImageCreateInfo &imageCreateInfo, VmaAllocation &allocation,
                                 vk::Image &textureImage, const std::string &allocationName);

    static vk::ImageView CreateImageView(vk::Device &device, const vk::ImageViewCreateInfo &imageCreateInfo);

    static std::unordered_map<vk::Format, vk::ImageView> CreateImageViews(
        vk::Device &device, const std::vector<vk::ImageViewCreateInfo> &imageCreateInfos);

    friend class Builder;
};
} // namespace star