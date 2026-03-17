#pragma once

#include "StarBuffers/Buffer.hpp"
#include "StarDescriptorBuilders.hpp"
#include "StarTextures/Texture.hpp"
#include "core/device/DeviceContext.hpp"

#include <star_common/Handle.hpp>
#include <star_common/HandleTypeRegistry.hpp>

#include <memory>
#include <optional>
#include <vector>

namespace star
{
class StarShaderInfo
{
  public:
    struct BufferInfo
    {
        BufferInfo(Handle handle, vk::Buffer currentBuffer) : handle(std::move(handle)), currentBuffer(currentBuffer)
        {
        }

        explicit BufferInfo(const StarBuffers::Buffer *buffer) : buffer(buffer)
        {
        }

        explicit BufferInfo(Handle handle) : handle(std::move(handle))
        {
        }

        std::optional<Handle> handle = std::nullopt;
        std::optional<const StarBuffers::Buffer *> buffer = std::nullopt;
        vk::Buffer currentBuffer{VK_NULL_HANDLE};
    };

    struct TextureInfo
    {
        TextureInfo(Handle handle, vk::ImageLayout expectedLayout)
            : handle(std::move(handle)), expectedLayout(expectedLayout)
        {
        }

        TextureInfo(Handle handle, vk::ImageLayout expectedLayout, vk::Format requestedImageViewFormat)
            : handle(std::move(handle)), expectedLayout(expectedLayout),
              requestedImageViewFormat(requestedImageViewFormat)
        {
        }

        TextureInfo(const StarTextures::Texture *texture, vk::ImageLayout expectedLayout)
            : texture(texture), expectedLayout(expectedLayout)
        {
        }

        TextureInfo(const StarTextures::Texture *texture, vk::ImageLayout expectedLayout,
                    vk::Format requestedImageViewFormat)
            : texture(texture), expectedLayout(expectedLayout), requestedImageViewFormat(requestedImageViewFormat)
        {
        }

        Handle handle;
        const StarTextures::Texture *texture{nullptr};
        vk::ImageLayout expectedLayout;
        std::optional<vk::Image> currentImage = std::nullopt;
        std::optional<vk::Format> requestedImageViewFormat = std::nullopt;
    };
    struct ShaderInfo
    {
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

        bool getWillCheckForIfReady() const
        {
            return m_willCheckForIfReady;
        }

      private:
        bool m_willCheckForIfReady;
    };

    struct ShaderInfoSet
    {
        std::vector<ShaderInfo> shaderInfos = std::vector<ShaderInfo>();

        ShaderInfoSet(core::device::StarDevice &device, StarDescriptorPool &pool, StarDescriptorSetLayout &setLayout)
            : device(device), m_pool(pool), setLayout(setLayout) {};

        void setNewResource(size_t index, ShaderInfo newInfo);

        void add(const ShaderInfo &shaderInfo);

        void buildIndex(const star::Handle &deviceID, size_t index);

        void build(const star::Handle &deviceID);

        vk::DescriptorSet getDescriptorSet(const Handle &deviceID);

        bool getIsBuilt() const
        {
            return this->isBuilt;
        }

      private:
        core::device::StarDevice &device;
        StarDescriptorPool &m_pool;
        StarDescriptorSetLayout &setLayout;
        std::vector<size_t> m_pendingBuildIndices; 
        bool setNeedsRebuild = true;
        bool isBuilt = false;
        std::shared_ptr<vk::DescriptorSet> descriptorSet = std::shared_ptr<vk::DescriptorSet>();
        std::shared_ptr<StarDescriptorWriter> descriptorWriter = std::shared_ptr<StarDescriptorWriter>();

        void rebuildSet();
    };
    class Builder
    {
      public:
        Builder(Handle deviceID, core::device::StarDevice &device, StarDescriptorPool &pool,
                const uint8_t numFramesInFlight)
            : m_deviceID(deviceID), device(device), m_pool(pool), sets(numFramesInFlight) {};

        Builder &addSetLayout(std::shared_ptr<StarDescriptorSetLayout> layout)
        {
            this->layouts.push_back(layout);
            return *this;
        };

        Builder &startOnFrameIndex(const size_t &frameInFlight)
        {
            assert(frameInFlight < this->sets.size() && "Pushed beyond size");
            this->activeSet = &this->sets[frameInFlight];
            return *this;
        }

        Builder &startSet();

        Builder &add(BufferInfo buffer, vk::Semaphore *resourceSemaphore = nullptr)
        {
            this->activeSet->back()->add(ShaderInfo(std::move(buffer), resourceSemaphore));
            return *this;
        };

        Builder &add(TextureInfo texture, vk::Semaphore *resourceSemaphore = nullptr)
        {
            this->activeSet->back()->add(ShaderInfo(std::move(texture), resourceSemaphore));
            return *this;
        }

        std::unique_ptr<StarShaderInfo> build()
        {
            return std::make_unique<StarShaderInfo>(m_deviceID, this->device, m_pool, std::move(this->layouts),
                                                    std::move(this->sets));
        };

        std::vector<std::shared_ptr<StarDescriptorSetLayout>> getCurrentSetLayouts()
        {
            return this->layouts;
        };

      private:
        star::Handle m_deviceID;
        core::device::StarDevice &device;
        StarDescriptorPool &m_pool;
        std::vector<std::shared_ptr<ShaderInfoSet>> *activeSet = nullptr;

        std::vector<std::shared_ptr<star::StarDescriptorSetLayout>> layouts =
            std::vector<std::shared_ptr<StarDescriptorSetLayout>>();
        std::vector<std::vector<std::shared_ptr<ShaderInfoSet>>> sets =
            std::vector<std::vector<std::shared_ptr<ShaderInfoSet>>>();
    };

    StarShaderInfo(Handle deviceID, core::device::StarDevice &device, StarDescriptorPool &pool,
                   std::vector<std::shared_ptr<StarDescriptorSetLayout>> layouts,
                   std::vector<std::vector<std::shared_ptr<ShaderInfoSet>>> shaderInfoSets)
        : m_deviceID(deviceID), layouts(std::move(layouts)), shaderInfoSets(std::move(shaderInfoSets)) {};

    bool isReady(uint8_t frameInFlight);

    void setNewResource(size_t frameInFlightIndex, size_t setIndex, size_t bindingIndex, TextureInfo texture); 

    void setNewResource(size_t frameInFlightIndex, size_t setIndex, size_t bindingIndex, BufferInfo buffer);

    std::vector<vk::DescriptorSetLayout> getDescriptorSetLayouts();

    std::vector<vk::DescriptorSet> getDescriptors(size_t frameInFlight);

    void cleanupRender(core::device::StarDevice &device);

    std::vector<std::vector<std::shared_ptr<ShaderInfoSet>>> &getShaderInfoSets()
    {
        return shaderInfoSets;
    }

  private:
    star::Handle m_deviceID;
    std::vector<std::shared_ptr<StarDescriptorSetLayout>> layouts;
    std::vector<std::vector<std::shared_ptr<ShaderInfoSet>>> shaderInfoSets;
};
} // namespace star