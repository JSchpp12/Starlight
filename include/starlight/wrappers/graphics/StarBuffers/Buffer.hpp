#pragma once

#include "Allocator.hpp"
#include "Enums.hpp"
#include "StarBuffers/Resources.hpp"
#include "device/StarDevice.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <string>

namespace star::StarBuffers
{

class Buffer
{
  public:
    class Builder
    {
      public:
        Builder(VmaAllocator &allocator);
        Builder &setAllocationCreateInfo(const VmaAllocationCreateInfo &nAllocInfo,
                                         const vk::BufferCreateInfo &nBufferInfo, const std::string &nAllocName);
        Builder &setInstanceCount(const uint32_t &nInstanceCount);
        Builder &setInstanceSize(const vk::DeviceSize &nInstanceSize);
        Builder &setMinOffsetAlignment(const vk::DeviceSize &nMinOffsetAlignment);
        std::unique_ptr<Buffer> buildUnique();
        Buffer build();

      private:
        VmaAllocator &allocator;
        vk::DeviceSize minOffsetAlignment = 1;
        std::string allocName;
        VmaAllocationCreateInfo allocInfo;
        vk::BufferCreateInfo buffInfo;
        vk::DeviceSize instanceSize = 0;
        uint32_t instanceCount = 0;
    };

    static vk::DeviceSize GetAlignment(const vk::DeviceSize &instanceSize, const vk::DeviceSize &minOffsetAlignment);

    Buffer() = default;
    Buffer(VmaAllocator &allocator, const uint32_t &instanceCount, const vk::DeviceSize &instanceSize,
           const vk::DeviceSize &minOffsetAlignment, const VmaAllocationCreateInfo &allocCreateInfo,
           const vk::BufferCreateInfo &bufferCreateInfo, const std::string &allocName);

    void map(void **mapped) const;

    void unmap() const;

    void writeToBuffer(void *data, void *mapped, const vk::DeviceSize &size = VK_WHOLE_SIZE,
                       const vk::DeviceSize &offset = 0);

    void cleanupRender(vk::Device &device);

    vk::Result flush(const vk::DeviceSize &size = VK_WHOLE_SIZE, const vk::DeviceSize &offset = 0) const;

    vk::DescriptorBufferInfo descriptorInfo(const vk::DeviceSize &size = VK_WHOLE_SIZE,
                                            const vk::DeviceSize &offset = 0);

    void writeToIndex(void *data, void *mapped, const size_t &index);
    vk::Result flushIndex(const size_t &index);
    vk::DescriptorBufferInfo descriptorInfoForIndex(const size_t &index);

    std::shared_ptr<Resources> releaseResources()
    {
        auto storage = std::move(resources);
        resources = nullptr;
        return storage;
    }

    const vk::Buffer &getVulkanBuffer() const
    {
        return this->resources->buffer;
    }
    const uint32_t &getInstanceCount() const
    {
        return instanceCount;
    }
    const vk::DeviceSize &getInstanceSize() const
    {
        return instanceSize;
    }
    const uint32_t &getAlignmentSize() const
    {
        return m_alignmentSize;
    }
    vk::BufferUsageFlags getUsageFlags() const
    {
        return usageFlags;
    }
    const vk::DeviceSize &getBufferSize() const
    {
        return size;
    }

  protected:
    std::shared_ptr<Resources> resources;
    vk::DeviceSize size;
    vk::DeviceSize offset;
    vk::DeviceSize instanceSize;
    uint32_t instanceCount;
    uint32_t m_alignmentSize;
    vk::BufferUsageFlags usageFlags;

    static std::shared_ptr<StarBuffers::Resources> CreateBuffer(VmaAllocator &allocator,
                                                                const VmaAllocationCreateInfo &allocCreateInfo,
                                                                const vk::BufferCreateInfo &bufferInfo,
                                                                vk::DeviceSize &bufferSize, vk::DeviceSize &offset,
                                                                const std::string &allocationName);

    static std::string AllocationName(const std::string &allocationName);

    static void LogAllocationFailure(const vk::Result &allocationResult);

    friend class Builder;
};
} // namespace star::StarBuffers