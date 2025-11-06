#pragma once

#include "TransferRequest_Texture.hpp"

#include <memory>
#include <random>
#include <vector>

namespace star::TransferRequest
{
template <typename TData, uint32_t TChannels> class TextureData : public Texture
{
  public:
    static constexpr vk::Format SelectFormat()
    {
        if constexpr (std::is_same_v<TData, uint8_t>)
        {
            if constexpr (TChannels == 4)
            {
                return vk::Format::eR8G8B8A8Unorm;
            }
            else if constexpr (TChannels == 3)
            {
                return vk::Format::eR8G8B8Unorm;
            }
            else if constexpr (TChannels == 2)
            {
                return vk::Format::eR8G8Unorm;
            }
            else if constexpr (TChannels == 1)
            {
                return vk::Format::eR8Unorm;
            }
            else
            {
                static_assert(false, "Unsupported channel count for type");
            }
        }
        else if constexpr (std::is_same_v<TData, float>)
        {
            if constexpr (TChannels == 4)
            {
                return vk::Format::eR32G32B32A32Sfloat;
            }
            else if constexpr (TChannels == 3)
            {
                return vk::Format::eR32G32B32Sfloat;
            }
            else if constexpr (TChannels == 2)
            {
                return vk::Format::eR32G32Sfloat;
            }
            else if constexpr (TChannels == 1)
            {
                return vk::Format::eR32Sfloat;
            }
            else
            {
                static_assert(false, "TData: Unsupported channel count for type");
            }
        }
        else
        {
            static_assert(false, "TData: Unsupported type");
        }
    }

    TextureData(uint32_t width, uint32_t height, uint32_t consumingQueueFamilyIndex,
                const vk::PhysicalDeviceProperties &deviceProps, vk::ImageLayout finalImageLayout)
        : m_width(std::move(width)), m_height(std::move(height)),
          m_consumingQueueFamilyIndex(std::move(consumingQueueFamilyIndex)),
          m_finalImageLayout(std::move(finalImageLayout))
    {
        static_assert(TChannels <= 4);
    }
    virtual ~TextureData() = default;
    TextureData(const TextureData &) = delete;
    TextureData &operator=(const TextureData &) = delete;
    TextureData(TextureData &&other)
        : m_width(std::move(other.m_width)), m_height(std::move(other.m_height)),
          m_consumingQueueFamilyIndex(std::move(other.m_consumingQueueFamilyIndex)),
          m_rawData(std::move(other.m_rawData))
    {
    }
    TextureData &operator=(TextureData &&other)
    {
        if (this != &other)
        {
            m_width = std::move(other.m_width);
            m_height = std::move(other.m_height);
            m_consumingQueueFamilyIndex = std::move(other.m_consumingQueueFamilyIndex);
            m_rawData = std::move(other.m_rawData);
        }

        return this;
    }

    virtual void prep() override
    {
        if (!m_rawData)
        {
            m_rawData = loadTexture(m_width, m_height);
        }
    }

    std::unique_ptr<StarBuffers::Buffer> createStagingBuffer(vk::Device &device, VmaAllocator &allocator) const override
    {
        const size_t size = getSizeOfData(); 

        return StarBuffers::Buffer::Builder(allocator)
            .setAllocationCreateInfo(
                Allocator::AllocationBuilder()
                    .setFlags(VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT)
                    .setUsage(VMA_MEMORY_USAGE_AUTO)
                    .build(),
                vk::BufferCreateInfo()
                    .setSharingMode(vk::SharingMode::eExclusive)
                    .setSize(size)
                    .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
                "GeneratedTextureData")
            .setInstanceCount(1)
            .setInstanceSize(size)
            .build();
    }

    std::unique_ptr<star::StarTextures::Texture> createFinal(
        vk::Device &device, VmaAllocator &allocator,
        const std::vector<uint32_t> &transferQueueFamilyIndex) const override
    {
        constexpr vk::Format baseFormat = SelectFormat();

        std::vector<uint32_t> indices = std::vector<uint32_t>{m_consumingQueueFamilyIndex};
        for (auto &index : transferQueueFamilyIndex)
        {
            indices.push_back(index);
        }

        uint32_t indexCount = 0;
        CastHelpers::SafeCast<size_t, uint32_t>(indices.size(), indexCount);

        return star::StarTextures::Texture::Builder(device, allocator)
            .setCreateInfo(Allocator::AllocationBuilder()
                               .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                               .setUsage(VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO)
                               .build(),
                           vk::ImageCreateInfo()
                               .setExtent(vk::Extent3D().setWidth(m_width).setHeight(m_height).setDepth(1))
                               .setUsage(vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst)
                               .setImageType(vk::ImageType::e2D)
                               .setMipLevels(1)
                               .setArrayLayers(1)
                               .setTiling(vk::ImageTiling::eOptimal)
                               .setInitialLayout(vk::ImageLayout::eUndefined)
                               .setSamples(vk::SampleCountFlagBits::e1)
                               .setPQueueFamilyIndices(indices.data())
                               .setQueueFamilyIndexCount(indexCount)
                               .setFormat(baseFormat),
                           "GeneratedTexture")
            .setBaseFormat(baseFormat)
            .addViewInfo(vk::ImageViewCreateInfo()
                             .setViewType(vk::ImageViewType::e2D)
                             .setFormat(baseFormat)
                             .setSubresourceRange(vk::ImageSubresourceRange()
                                                      .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                      .setBaseArrayLayer(0)
                                                      .setLayerCount(1)
                                                      .setBaseMipLevel(0)
                                                      .setLevelCount(1)))
            .buildUnique();
    }

    virtual void copyFromTransferSRCToDST(StarBuffers::Buffer &srcBuffer, star::StarTextures::Texture &dst,
                                          vk::CommandBuffer &commandBuffer) const override
    {
        star::StarTextures::Texture::TransitionImageLayout(
            dst, commandBuffer, dst.getBaseFormat(), vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

        auto cpyRegion = vk::BufferImageCopy()
                             .setBufferOffset(0)
                             .setBufferRowLength(0)
                             .setBufferImageHeight(0)
                             .setImageExtent(vk::Extent3D{m_width, m_height, 1})
                             .setImageSubresource(vk::ImageSubresourceLayers()
                                                      .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                      .setMipLevel(0)
                                                      .setBaseArrayLayer(0)
                                                      .setLayerCount(1));

        commandBuffer.copyBufferToImage(srcBuffer.getVulkanBuffer(), dst.getVulkanImage(),
                                        vk::ImageLayout::eTransferDstOptimal, cpyRegion);

        StarTextures::Texture::TransitionImageLayout(dst, commandBuffer, dst.getBaseFormat(),
                                                     vk::ImageLayout::eTransferDstOptimal, m_finalImageLayout);
    }

    virtual void writeDataToStageBuffer(star::StarBuffers::Buffer &stagingBuffer) const override
    {
        assert(m_rawData && "Data needs to be loaded before trying to write to buffers");


        const size_t size = getSizeOfData(); 

        void *mapped = nullptr;
        stagingBuffer.map(&mapped);
        stagingBuffer.writeToBuffer(m_rawData->data(), mapped, size);
        stagingBuffer.unmap();
    }

  protected:
    virtual std::unique_ptr<std::vector<TData>> loadTexture(const uint32_t &width, const uint32_t &height) const = 0;

    size_t getSizeOfData() const {
        assert(m_rawData && "Data must be loaded before size can be computed"); 
        return m_rawData->size() * sizeof(TData); 
    }

  private:
    uint32_t m_width, m_height, m_consumingQueueFamilyIndex;
    vk::ImageLayout m_finalImageLayout;
    std::unique_ptr<std::vector<TData>> m_rawData = nullptr;
};
} // namespace star::TransferRequest