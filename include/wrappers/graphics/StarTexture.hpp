#pragma once

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

#include <unordered_map>
#include <optional>
namespace star{
    class StarTexture{
        public:
        /// <summary>
        /// Options to be used when creating a new texture
        /// </summary>
        /// <returns></returns>
        struct TextureCreateSettings {
            TextureCreateSettings() = default;

            TextureCreateSettings(const int& width, 
                const int& height, const int& channels, 
                const int& depth, const int& byteDepth,
                const vk::ImageUsageFlags& imageUsage, const vk::Format& imageFormat,
                const vk::ImageAspectFlags& imageAspectFlags, const VmaMemoryUsage& memoryUsage,
                const VmaAllocationCreateFlags& allocationCreateFlags, const vk::ImageLayout& initialLayout, 
                const bool& isMutable, const bool& createSampler, const vk::MemoryPropertyFlags& requiredMemoryProperties, 
                const float& anisotropyLevel, const vk::Filter& textureFilteringMode, const std::string& allocationName) 
                : width(width), height(height), channels(channels), depth(depth), byteDepth(byteDepth),
                imageUsage(imageUsage), imageFormat(imageFormat), 
                allocationCreateFlags(allocationCreateFlags), memoryUsage(memoryUsage), 
                isMutable(isMutable), createSampler(createSampler), initialLayout(initialLayout), 
                aspectFlags(imageAspectFlags), requiredMemoryProperties(requiredMemoryProperties), 
                anisotropyLevel(anisotropyLevel), textureFilteringMode(textureFilteringMode), allocationName(allocationName){ }

            ~TextureCreateSettings() = default; 

            TextureCreateSettings(const TextureCreateSettings& creatSettings) = default;

            bool createSampler = false; 
            bool isMutable = false;
            int height, width, channels, depth, byteDepth = 0; 
            vk::ImageUsageFlags imageUsage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
            vk::Format imageFormat = vk::Format::eR8G8B8A8Srgb;
            VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
            VmaAllocationCreateFlags allocationCreateFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT & VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;
            vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor;
            vk::ImageLayout initialLayout = vk::ImageLayout::eShaderReadOnlyOptimal; 
            std::optional<vk::MemoryPropertyFlags> requiredMemoryProperties = std::nullopt; 
            float anisotropyLevel = 1.0f;
            vk::Filter textureFilteringMode = vk::Filter::eNearest;
            std::string allocationName = "TextureDefaultName";
        }; 

        virtual ~StarTexture();
        StarTexture(const TextureCreateSettings& createSettings, vk::Device& device, VmaAllocator& allocator); 
        StarTexture(const TextureCreateSettings& createSettings, vk::Device& device, const vk::Image& textureImage);

        void transitionLayout(vk::CommandBuffer& commandBuffer, vk::ImageLayout newLayout, vk::AccessFlags srcFlags, 
            vk::AccessFlags dstFlags, vk::PipelineStageFlags sourceStage,
            vk::PipelineStageFlags dstStage);

        //TODO: might want to remove the image layout stuff
        vk::ImageLayout getCurrentLayout() const {return this->layout;}
        void overrideImageLayout(vk::ImageLayout newLayout) {this->layout = newLayout;}

        vk::Sampler getSampler() const {return this->textureSampler;}
        vk::ImageView getImageView(const vk::Format* requestedFormat = nullptr) const; 
        const TextureCreateSettings& getCreationSettings() const { return this->createSettings; }

        vk::Image getImage() const {return this->textureImage;}
        const int getWidth() const { return this->createSettings.width; };
        const int getHeight() const { return this->createSettings.height; };
        const int getChannels() const { return this->createSettings.channels; }
        const int getDepth() const { return this->createSettings.depth; }
        const bool isVulkanImageReady() const {return this->textureImage;}

        protected:
        const TextureCreateSettings createSettings;
        vk::Device& device; 
        VmaAllocator* allocator = nullptr;
        VmaAllocation textureMemory = VmaAllocation();
        vk::Image textureImage = vk::Image();
        vk::Sampler textureSampler = vk::Sampler();
        std::unordered_map<vk::Format, vk::ImageView> imageViews = std::unordered_map<vk::Format, vk::ImageView>(); 
        vk::ImageLayout layout = vk::ImageLayout::eUndefined;

        static void createAllocation(const TextureCreateSettings& createSettings, VmaAllocator& allocator, VmaAllocation& textureMemory, vk::Image& texture);

        void createTextureImageView(const vk::Format& viewFormat, const vk::ImageAspectFlags& aspectFlags);

        vk::ImageView createImageView(vk::Device& device, vk::Image image, vk::Format format, const vk::ImageAspectFlags& aspectFlags); 

        static vk::Sampler createImageSampler(vk::Device& device, const TextureCreateSettings& createSettings); 
    };
}