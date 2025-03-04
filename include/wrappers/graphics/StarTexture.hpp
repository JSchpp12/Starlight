#pragma once

#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>

namespace star{
    class StarTexture{
        public:
        /// <summary>
        /// Options to be used when creating a new texture
        /// </summary>
        /// <returns></returns>
        struct TextureCreateSettings {
            TextureCreateSettings(const int& width, 
                const int& height, const int& channels, 
                const int& depth, const int& byteDepth,
                const vk::ImageUsageFlags& imageUsage, const vk::Format& imageFormat,
                const vk::ImageAspectFlags& imageAspectFlags, const VmaMemoryUsage& memoryUsage,
                const VmaAllocationCreateFlags& allocationCreateFlags, const vk::ImageLayout& initialLayout, 
                const bool& isMutable, const bool& createSampler) 
                : width(width), height(height), channels(channels), depth(depth), byteDepth(byteDepth),
                imageUsage(imageUsage), imageFormat(imageFormat), 
                allocationCreateFlags(allocationCreateFlags), memoryUsage(memoryUsage), 
                isMutable(isMutable), createSampler(createSampler), initialLayout(initialLayout), aspectFlags(imageAspectFlags){ }

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
        }; 

        StarTexture(const TextureCreateSettings& createSettings); 
        
        const TextureCreateSettings& getCreationSettings() const { return this->createSettings; }
        const int getWidth() const { return this->createSettings.width; };
        const int getHeight() const { return this->createSettings.height; };
        const int getChannels() const { return this->createSettings.channels; }
        const int getDepth() const { return this->createSettings.depth; }
        protected:
        const TextureCreateSettings createSettings;

        private:

    };
}