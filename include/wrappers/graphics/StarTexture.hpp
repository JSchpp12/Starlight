#pragma once

#include "StarDevice.hpp"

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

#include <unordered_map>
#include <optional>
#include <vector> 

namespace star{
    class StarTexture{
        public:
        static float SelectAnisotropyLevel(const vk::PhysicalDeviceProperties& deviceProperties); 

        static vk::Filter SelectTextureFiltering(const vk::PhysicalDeviceProperties& deviceProperties); 
        class Builder{
            public:
                Builder(const vk::Image& vulkanImage) : vulkanImage(vulkanImage){}; 

                Builder(StarDevice& device, VmaAllocator& allocator) 
                : createNewAllocationInfo{std::make_optional<Creators>(device, allocator)}{

                }

                Builder& setCreateInfo(const VmaAllocationCreateInfo& nAllocInfo, const vk::ImageCreateInfo& nCreateInfo){
                    assert(this->createNewAllocationInfo.has_value() && "Must be a builder for a new allocation");

                    this->createNewAllocationInfo.value().createInfo = nCreateInfo; 
                    this->createNewAllocationInfo.value().allocationCreateInfo = nAllocInfo;
                    return *this;
                }
                Builder& setViewInfo(const vk::ImageViewCreateInfo& nCreateInfo){
                    this->viewInfos.push_back(nCreateInfo); 
                    return *this;
                }
                Builder& setSamplerInfo(const vk::SamplerCreateInfo& nCreateInfo){
                    this->samplerInfo = nCreateInfo; 
                    return *this;
                }
                std::unique_ptr<StarTexture> build(){
                    if (this->createNewAllocationInfo.has_value()){

                    }else if (this->vulkanImage.has_value()){

                    }else{
                        std::err << "Invalid builder config" << std::endl;
                        throw std::runtime_error("Invalid builder config"); 
                    }
                    return nullptr; 
                }

            private: 
            struct Creators{
                Creators(StarDevice& device, VmaAllocator& allocator) : device(device), allocator(allocator){}
                
                StarDevice& device; 
                VmaAllocator& allocator; 
                vk::ImageCreateInfo createInfo = vk::ImageCreateInfo(); 
                VmaAllocationCreateInfo allocationCreateInfo = VmaAllocationCreateInfo(); 
            };

            std::optional<Creators> createNewAllocationInfo = std::nullopt; 
            std::optional<vk::Image> vulkanImage = std::nullopt; 

            std::vector<vk::ImageViewCreateInfo> viewInfos = std::vector<vk::ImageViewCreateInfo>(); 
            std::optional<vk::SamplerCreateInfo> samplerInfo = std::nullopt; 
        };


        virtual ~StarTexture();

        void transitionLayout(vk::CommandBuffer& commandBuffer, vk::ImageLayout newLayout, vk::AccessFlags srcFlags, 
            vk::AccessFlags dstFlags, vk::PipelineStageFlags sourceStage,
            vk::PipelineStageFlags dstStage);

        //TODO: might want to remove the image layout stuff
        vk::ImageLayout getCurrentLayout() const {return this->layout;}
        void overrideImageLayout(vk::ImageLayout newLayout) {this->layout = newLayout;}

        vk::Sampler getSampler() const {return this->textureSampler;}
        vk::ImageView getImageView(const vk::Format* requestedFormat = nullptr) const; 
        const RawTextureCreateSettings& getCreationSettings() const { return this->createSettings; }

        vk::Image getImage() const {return this->textureImage;}
        const int getWidth() const { return this->createSettings.width; };
        const int getHeight() const { return this->createSettings.height; };
        const int getChannels() const { return this->createSettings.channels; }
        const int getDepth() const { return this->createSettings.depth; }
        const int getMipMapLevels() const {return this->createSettings.mipmapInfo.value().numLevels;}
        const bool isVulkanImageReady() const {return this->textureImage;}
        vk::DeviceSize getImageMemorySize() const; 

        protected:
        StarTexture(StarDevice& device, vk::Image& vulkanImage)
        StarTexture(StarDevice& device, VmaAllocator& allocator, 
            const vk::ImageCreateInfo& createInfo, const VmaAllocationCreateInfo& allocationCreateInfo, 
            const std::vector<vk::ImageViewCreateInfo>& imageViewInfos);

        StarTexture(StarDevice& device, VmaAllocator& allocator, 
            const vk::ImageCreateInfo& createInfo, const VmaAllocationCreateInfo& allocationCreateInfo, 
            const std::vector<vk::ImageViewCreateInfo>& imageViewInfos, const vk::SamplerCreateInfo& samplerInfo);

        const RawTextureCreateSettings createSettings;
        vk::Device& device; 
        VmaAllocator* allocator = nullptr;
        VmaAllocation textureMemory = VmaAllocation();
        vk::Image textureImage = vk::Image();
        vk::Sampler textureSampler = vk::Sampler();
        std::unordered_map<vk::Format, vk::ImageView> imageViews = std::unordered_map<vk::Format, vk::ImageView>(); 
        vk::ImageLayout layout = vk::ImageLayout::eUndefined;

        static void createAllocation(const RawTextureCreateSettings& createSettings, VmaAllocator& allocator, 
            VmaAllocation& textureMemory, vk::Image& texture, const bool& isMutable);

        void createTextureImageView(const vk::Format& viewFormat, const vk::ImageAspectFlags& aspectFlags);

        vk::ImageView createImageView(vk::Device& device, vk::Image image, vk::Format format, const vk::ImageAspectFlags& aspectFlags, const int& mipMapLevels); 

        static vk::Sampler createImageSampler(vk::Device& device, const RawTextureCreateSettings& createSettings); 

        friend class Builder;
        
    };
}