#pragma once

#include "StarBuffer.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>

namespace star{
    class BufferMemoryTransferRequest {
    public:
    struct BufferCreationArgs {
        vk::DeviceSize instanceSize;
        uint32_t instanceCount;
        VmaAllocationCreateFlags creationFlags;
        VmaMemoryUsage memoryUsageFlags;
        vk::BufferUsageFlags useFlags;
        vk::SharingMode sharingMode;

        BufferCreationArgs(const vk::DeviceSize& instanceSize,
            const uint32_t& instanceCount, const VmaAllocationCreateFlags& creationFlags, const VmaMemoryUsage& memoryUsageFlags,
            const vk::BufferUsageFlags& useFlags, const vk::SharingMode& sharingMode) 
            : instanceSize(instanceSize), instanceCount(instanceCount), creationFlags(creationFlags), memoryUsageFlags(memoryUsageFlags),
            useFlags(useFlags), sharingMode(sharingMode){};
    };

    BufferMemoryTransferRequest() = default;

    virtual BufferCreationArgs getCreateArgs() const = 0;
    
    virtual void writeData(StarBuffer& buffer) const = 0; 

    protected:

    };
}
