#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace star::StarBuffers
{
struct Resources
{
    Resources(VmaAllocator &allocator, VmaAllocation &memory, vk::Buffer &buffer)
        : allocator(allocator), memory(memory), buffer(buffer)
    {
    }

    ~Resources(){
        if (this->buffer != VK_NULL_HANDLE)
            vmaDestroyBuffer(this->allocator, this->buffer, this->memory);
    }

    VmaAllocator allocator;
    VmaAllocation memory;
    vk::Buffer buffer   = VK_NULL_HANDLE;
};
} // namespace star::StarBuffers