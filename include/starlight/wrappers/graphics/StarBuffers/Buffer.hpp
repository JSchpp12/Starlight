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

    static vk::DeviceSize GetAlignment(vk::DeviceSize instanceSize, vk::DeviceSize minOffsetAlignment);

    Buffer() = default;
    Buffer(VmaAllocator &allocator, const uint32_t &instanceCount, const vk::DeviceSize &instanceSize,
           const vk::DeviceSize &minOffsetAlignment, const VmaAllocationCreateInfo &allocCreateInfo,
           const vk::BufferCreateInfo &bufferCreateInfo, const std::string &allocName);

    ~Buffer();

    Buffer(const Buffer &) = default;
    Buffer(Buffer &&other) = default;
    Buffer &operator=(const Buffer &) = default;
    Buffer &operator=(Buffer &&other) = default;

    void map(void **mapped, vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);

    void unmap();

    void writeToBuffer(void *data, void *mapped, vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);

    void cleanupRender(vk::Device &device)
    {
        if (resources)
        {
            resources->cleanupRender();
        }
    }

    vk::Result flush(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);

    vk::DescriptorBufferInfo descriptorInfo(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);

    void writeToIndex(void *data, void *mapped, int index);
    vk::Result flushIndex(int index);
    vk::DescriptorBufferInfo descriptorInfoForIndex(int index);

    std::shared_ptr<Resources> releaseResources()
    {
        auto storage = std::move(resources);
        resources = nullptr;
        return storage;
    }

    vk::Buffer getVulkanBuffer() const
    {
        return this->resources->buffer;
    }
    uint32_t getInstanceCount() const
    {
        return instanceCount;
    }
    vk::DeviceSize getInstanceSize() const
    {
        return instanceSize;
    }
    vk::DeviceSize getAlignmentSize() const
    {
        return alignmentSize;
    }
    vk::BufferUsageFlags getUsageFlags() const
    {
        return usageFlags;
    }
    vk::DeviceSize getBufferSize() const
    {
        return size;
    }

  protected:
    std::shared_ptr<Resources> resources;
    vk::DeviceSize size;
    vk::DeviceSize offset;
    vk::DeviceSize instanceSize;
    uint32_t instanceCount;
    uint32_t alignmentSize;
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