#pragma once

#include "DeviceContext.hpp"
#include "StarBuffers/Buffer.hpp"
#include "StarDescriptorBuilders.hpp"
#include "StarTextures/Texture.hpp"

#include "Handle.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace star
{
class StarShaderInfo
{
  private:
    struct ShaderInfo
    {
        struct BufferInfo
        {
            BufferInfo(const Handle &handle, const vk::Buffer &currentBuffer)
                : handle(handle), currentBuffer(currentBuffer)
            {
            }

            BufferInfo(const StarBuffers::Buffer *buffer) : buffer(buffer)
            {
            }

            explicit BufferInfo(const Handle &handle) : handle(handle)
            {
            }

            std::optional<Handle> handle = std::nullopt;
            std::optional<const StarBuffers::Buffer *> buffer = std::nullopt;
            std::optional<vk::Buffer> currentBuffer = std::nullopt;
        };

        struct TextureInfo
        {
            TextureInfo(const Handle &handle, const vk::ImageLayout &expectedLayout)
                : handle(handle), expectedLayout(expectedLayout)
            {
            }

            TextureInfo(const Handle &handle, const vk::ImageLayout &expectedLayout,
                        const vk::Format &requestedImageViewFormat)
                : handle(handle), expectedLayout(expectedLayout), requestedImageViewFormat(requestedImageViewFormat)
            {
            }

            TextureInfo(const StarTextures::Texture *texture, const vk::ImageLayout &expectedLayout)
                : texture(texture), expectedLayout(expectedLayout)
            {
            }

            TextureInfo(const StarTextures::Texture *texture, const vk::ImageLayout &expectedLayout,
                        const vk::Format &requestedImageViewFormat)
                : texture(texture), expectedLayout(expectedLayout), requestedImageViewFormat(requestedImageViewFormat)
            {
            }

            std::optional<const Handle> handle = std::nullopt;
            std::optional<const StarTextures::Texture *> texture = std::nullopt;
            const vk::ImageLayout expectedLayout;
            std::optional<vk::Image> currentImage = std::nullopt;
            std::optional<vk::Format> requestedImageViewFormat = std::nullopt;
        };

        ShaderInfo(const BufferInfo &bufferInfo, vk::Semaphore *resourceSemaphore = nullptr)
            : bufferInfo(bufferInfo),
              m_resourceSemaphore(resourceSemaphore != nullptr ? vk::Semaphore(*resourceSemaphore) : VK_NULL_HANDLE),
              m_willCheckForIfReady(resourceSemaphore != nullptr ? false : true)
        {
        }

        ShaderInfo(const TextureInfo &textureInfo, vk::Semaphore *resourceSemaphore = nullptr)
            : textureInfo(textureInfo),
              m_resourceSemaphore(resourceSemaphore != nullptr ? vk::Semaphore(*resourceSemaphore) : VK_NULL_HANDLE),
              m_willCheckForIfReady(resourceSemaphore != nullptr ? false : true)
        {
        }

        ~ShaderInfo() = default;

        std::optional<BufferInfo> bufferInfo = std::nullopt;
        std::optional<TextureInfo> textureInfo = std::nullopt;
        vk::Semaphore m_resourceSemaphore;
        const bool m_willCheckForIfReady;
    };

    struct ShaderInfoSet
    {
        std::vector<ShaderInfo> shaderInfos = std::vector<ShaderInfo>();

        ShaderInfoSet(core::device::StarDevice &device, StarDescriptorSetLayout &setLayout)
            : device(device), setLayout(setLayout) {};

        void add(const ShaderInfo &shaderInfo);

        void buildIndex(const core::device::DeviceID &deviceID, const int &index);

        void build(const core::device::DeviceID &deviceID);

        vk::DescriptorSet getDescriptorSet();

        bool getIsBuilt() const
        {
            return this->isBuilt;
        }

      private:
        core::device::StarDevice &device;
        StarDescriptorSetLayout &setLayout;
        bool setNeedsRebuild = true;
        bool isBuilt = false;
        std::shared_ptr<vk::DescriptorSet> descriptorSet = std::shared_ptr<vk::DescriptorSet>();
        std::shared_ptr<StarDescriptorWriter> descriptorWriter = std::shared_ptr<StarDescriptorWriter>();

        void rebuildSet();
    };

    std::vector<std::shared_ptr<StarDescriptorSetLayout>> layouts =
        std::vector<std::shared_ptr<StarDescriptorSetLayout>>();
    std::vector<std::vector<std::shared_ptr<ShaderInfoSet>>> shaderInfoSets =
        std::vector<std::vector<std::shared_ptr<ShaderInfoSet>>>();

  public:
    StarShaderInfo(core::device::DeviceID deviceID, core::device::StarDevice &device,
                   const std::vector<std::shared_ptr<StarDescriptorSetLayout>> &layouts,
                   const std::vector<std::vector<std::shared_ptr<ShaderInfoSet>>> &shaderInfoSets)
        : m_deviceID(deviceID), layouts(layouts), shaderInfoSets(shaderInfoSets) {};

    bool isReady(const uint8_t &frameInFlight);

    std::set<vk::Semaphore> getDependentSemaphores(const uint8_t &frameInFlight); 

    std::vector<vk::DescriptorSetLayout> getDescriptorSetLayouts();

    std::vector<vk::DescriptorSet> getDescriptors(const int &frameInFlight);

    void cleanupRender(core::device::StarDevice &device);

    class Builder
    {
      public:
        Builder(core::device::DeviceID deviceID, core::device::StarDevice &device, const int numFramesInFlight)
            : m_deviceID(deviceID), device(device), sets(numFramesInFlight) {};

        Builder &addSetLayout(std::shared_ptr<StarDescriptorSetLayout> layout)
        {
            this->layouts.push_back(layout);
            return *this;
        };

        Builder &startOnFrameIndex(const int &frameInFlight)
        {
            assert(frameInFlight < this->sets.size() && "Pushed beyond size");
            this->activeSet = &this->sets[frameInFlight];
            return *this;
        }

        Builder &startSet();

        Builder &add(const Handle &bufferHandle, vk::Semaphore *resourceSemaphore = nullptr)
        {
            assert(bufferHandle.getType() == star::Handle_Type::buffer);

            this->activeSet->back()->add(ShaderInfo(ShaderInfo::BufferInfo{bufferHandle}, resourceSemaphore));
            return *this;
        };

        Builder &add(const StarBuffers::Buffer &buffer)
        {
            this->activeSet->back()->add(ShaderInfo(ShaderInfo::BufferInfo{&buffer}));
            return *this;
        };

        Builder &add(const StarTextures::Texture &texture, const vk::ImageLayout &desiredLayout,
                     const vk::Format &requestedImageViewFormat, vk::Semaphore *resourceSemaphore = nullptr)
        {
            this->activeSet->back()->add(ShaderInfo(
                ShaderInfo::TextureInfo{&texture, desiredLayout, requestedImageViewFormat}, resourceSemaphore));
            return *this;
        };

        Builder &add(const StarTextures::Texture &texture, const vk::ImageLayout &desiredLayout,
                     vk::Semaphore *resourceSemaphore = nullptr)
        {
            this->activeSet->back()->add(
                ShaderInfo(ShaderInfo::TextureInfo{&texture, desiredLayout}, resourceSemaphore));
            return *this;
        }

        Builder &add(const Handle &textureHandle, const vk::ImageLayout &desiredLayout, vk::Semaphore *resourceSemaphore  = nullptr)
        {
            assert(textureHandle.getType() == Handle_Type::texture);
            this->activeSet->back()->add(
                ShaderInfo(ShaderInfo::TextureInfo{textureHandle, desiredLayout}, resourceSemaphore));
            return *this;
        }

        Builder &add(const Handle &textureHandle, const vk::ImageLayout &desiredLayout,
                     const vk::Format &requestedImageViewFormat, vk::Semaphore *resourceSemaphore = nullptr)
        {
            assert(textureHandle.getType() == Handle_Type::texture);

            this->activeSet->back()->add(
                ShaderInfo(ShaderInfo::TextureInfo{textureHandle, desiredLayout, requestedImageViewFormat},
                           resourceSemaphore));
            return *this;
        }

        std::unique_ptr<StarShaderInfo> build()
        {
            return std::make_unique<StarShaderInfo>(m_deviceID, this->device, std::move(this->layouts),
                                                    std::move(this->sets));
        };

        std::vector<std::shared_ptr<StarDescriptorSetLayout>> getCurrentSetLayouts()
        {
            return this->layouts;
        };

      private:
        core::device::DeviceID m_deviceID;
        core::device::StarDevice &device;
        std::vector<std::shared_ptr<ShaderInfoSet>> *activeSet = nullptr;

        std::vector<std::shared_ptr<star::StarDescriptorSetLayout>> layouts =
            std::vector<std::shared_ptr<StarDescriptorSetLayout>>();
        std::vector<std::vector<std::shared_ptr<ShaderInfoSet>>> sets =
            std::vector<std::vector<std::shared_ptr<ShaderInfoSet>>>();
    };

  private:
    core::device::DeviceID m_deviceID;
};
} // namespace star