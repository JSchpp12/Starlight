#pragma once

#include "Allocator.hpp"
#include "Enums.hpp"
#include "StarBuffers/Resources.hpp"
#include "device/StarDevice.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace star::StarBuffers
{

class Buffer
{
  public:
    struct Creators
    {
        Creators() = default;
        Creators(vk::BufferCreateInfo createInfo, VmaAllocationCreateInfo allocationCreateInfo,
                 std::string allocationName)
            : createInfo(std::move(createInfo)), allocationCreateInfo(std::move(allocationCreateInfo)),
              allocationName(std::move(allocationName))
        {
        }
        vk::BufferCreateInfo createInfo = vk::BufferCreateInfo();
        VmaAllocationCreateInfo allocationCreateInfo = VmaAllocationCreateInfo();
        std::string allocationName = "Default Buffer Name";
    };

    Buffer() = default;
    Buffer(std::shared_ptr<Resources> resources, const uint32_t &instanceCount, const vk::DeviceSize &instanceSize,
           const vk::DeviceSize &alignmentSize, const vk::DeviceSize &offset, const vk::DeviceSize &size,
           const vk::BufferUsageFlags &usageFlags);

    void map(void **mapped) const;

    void unmap() const;

    void writeToBuffer(void *data, void *mapped, const vk::DeviceSize &size = vk::WholeSize,
                       const vk::DeviceSize &offset = 0);

    void cleanupRender(vk::Device &device);

    vk::Result invalidate(const vk::DeviceSize &size = vk::WholeSize, const vk::DeviceSize &offset = 0) const;
    vk::Result flush(const vk::DeviceSize &size = vk::WholeSize, const vk::DeviceSize &offset = 0) const;

    vk::DescriptorBufferInfo descriptorInfo(const vk::DeviceSize &size = vk::WholeSize,
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

    vk::Buffer getVulkanBuffer() const
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
    const vk::DeviceSize &getAlignmentSize() const
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
    std::shared_ptr<Resources> resources = nullptr;
    vk::DeviceSize size = 0;
    vk::DeviceSize offset = 0;
    vk::DeviceSize instanceSize = 0;
    uint32_t instanceCount = 0;
    vk::DeviceSize m_alignmentSize = 0;
    vk::BufferUsageFlags usageFlags = vk::BufferUsageFlags();
};
} // namespace star::StarBuffers
