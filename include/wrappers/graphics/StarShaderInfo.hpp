#pragma once

#include "StarBuffer.hpp"
#include "StarDescriptorBuilders.hpp"
#include "StarDevice.hpp"
#include "StarTexture.hpp"


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

            BufferInfo(const StarBuffer *buffer) : buffer(buffer)
            {
            }

            explicit BufferInfo(const Handle &handle) : handle(handle)
            {
            }
            std::optional<const StarBuffer *> buffer = std::nullopt;
            std::optional<Handle> handle = std::nullopt;
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

            TextureInfo(const StarTexture *texture, const vk::ImageLayout &expectedLayout)
                : texture(texture), expectedLayout(expectedLayout)
            {
            }

            TextureInfo(const StarTexture *texture, const vk::ImageLayout &expectedLayout,
                        const vk::Format &requestedImageViewFormat)
                : texture(texture), expectedLayout(expectedLayout), requestedImageViewFormat(requestedImageViewFormat)
            {
            }

            std::optional<const Handle> handle = std::nullopt;
            std::optional<vk::Image> currentImage = std::nullopt;
            std::optional<vk::Format> requestedImageViewFormat = std::nullopt;
            std::optional<const StarTexture *> texture = std::nullopt;
            const vk::ImageLayout expectedLayout;
        };

        ShaderInfo(const BufferInfo &bufferInfo, const bool willCheckForIfReady)
            : bufferInfo(bufferInfo), willCheckForIfReady(willCheckForIfReady)
        {
        }

        ShaderInfo(const TextureInfo &textureInfo, const bool willCheckForIfReady)
            : textureInfo(textureInfo), willCheckForIfReady(willCheckForIfReady)
        {
        }

        ~ShaderInfo() = default;

        std::optional<BufferInfo> bufferInfo = std::nullopt;
        std::optional<TextureInfo> textureInfo = std::nullopt;
        const bool willCheckForIfReady;
    };

    struct ShaderInfoSet
    {
        std::vector<ShaderInfo> shaderInfos = std::vector<ShaderInfo>();

        ShaderInfoSet(StarDevice &device, StarDescriptorSetLayout &setLayout) : device(device), layout(setLayout) {};

        void add(const ShaderInfo &shaderInfo);

        void buildIndex(const int &index);

        void build();

        vk::DescriptorSet getDescriptorSet();

        bool getIsBuilt() const
        {
            return this->isBuilt;
        }

      private:
        StarDevice &device;
        StarDescriptorSetLayout &layout;
        bool setNeedsRebuild = true;
        bool isBuilt = false;
        std::shared_ptr<vk::DescriptorSet> descriptorSet = std::shared_ptr<vk::DescriptorSet>();
        std::shared_ptr<StarDescriptorWriter> descriptorWriter = std::shared_ptr<StarDescriptorWriter>();

        void rebuildSet();
    };

    std::vector<std::vector<std::shared_ptr<ShaderInfoSet>>> shaderInfoSets =
        std::vector<std::vector<std::shared_ptr<ShaderInfoSet>>>();
    std::vector<std::shared_ptr<StarDescriptorSetLayout>> layouts =
        std::vector<std::shared_ptr<StarDescriptorSetLayout>>();

  public:
    StarShaderInfo(StarDevice &device, const std::vector<std::shared_ptr<StarDescriptorSetLayout>> &layouts,
                   const std::vector<std::vector<std::shared_ptr<ShaderInfoSet>>> &shaderInfoSets)
        : layouts(layouts), shaderInfoSets(shaderInfoSets) {};

    bool isReady(const uint8_t &frameInFlight);

    std::vector<vk::DescriptorSetLayout> getDescriptorSetLayouts();

    std::vector<vk::DescriptorSet> getDescriptors(const int &frameInFlight);

    class Builder
    {
      public:
        Builder(StarDevice &device, const int numFramesInFlight) : device(device), sets(numFramesInFlight) {};

        Builder(Builder &other) : device(other.device)
        {
            this->layouts = other.layouts;
            this->sets = other.sets;
        };

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

        Builder &add(const Handle &bufferHandle, const bool willCheckForIfReady)
        {
            this->activeSet->back()->add(ShaderInfo(ShaderInfo::BufferInfo{bufferHandle}, willCheckForIfReady));
            return *this;
        };

        Builder &add(const StarBuffer &buffer)
        {
            this->activeSet->back()->add(ShaderInfo(ShaderInfo::BufferInfo{&buffer}, false));
            return *this;
        };

        Builder &add(const StarTexture &texture, const vk::ImageLayout &desiredLayout,
                     const vk::Format &requestedImageViewFormat, const bool willCheckForIfReady)
        {
            this->activeSet->back()->add(ShaderInfo(
                ShaderInfo::TextureInfo{&texture, desiredLayout, requestedImageViewFormat}, willCheckForIfReady));
            return *this;
        };

        Builder &add(const StarTexture &texture, const vk::ImageLayout &desiredLayout, const bool &willCheckForIfReady)
        {
            this->activeSet->back()->add(
                ShaderInfo(ShaderInfo::TextureInfo{&texture, desiredLayout}, willCheckForIfReady));
            return *this;
        }

        Builder &add(const Handle &textureHandle, const vk::ImageLayout &desiredLayout,
                     const bool &willCheckForIfReady);

        Builder &add(const Handle &textureHandle, const vk::ImageLayout &desiredLayout,
                     vk::Format &requestedImageViewFormat, const bool &willCheckForIfReady);

        std::unique_ptr<StarShaderInfo> build()
        {
            return std::make_unique<StarShaderInfo>(this->device, std::move(this->layouts), std::move(this->sets));
        };

        std::vector<std::shared_ptr<StarDescriptorSetLayout>> getCurrentSetLayouts()
        {
            return this->layouts;
        };

      private:
        StarDevice &device;
        std::vector<std::shared_ptr<ShaderInfoSet>> *activeSet = nullptr;

        std::vector<std::shared_ptr<star::StarDescriptorSetLayout>> layouts =
            std::vector<std::shared_ptr<StarDescriptorSetLayout>>();
        std::vector<std::vector<std::shared_ptr<ShaderInfoSet>>> sets =
            std::vector<std::vector<std::shared_ptr<ShaderInfoSet>>>();
    };
};
} // namespace star