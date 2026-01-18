#pragma once

#include "core/device/StarDevice.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace star::StarBuffers
{
struct Resources
{
    Resources(VmaAllocator allocator, VmaAllocation memory, vk::Buffer buffer)
        : allocator(std::move(allocator)), memory(std::move(memory)), buffer(std::move(buffer))
    {
    }

    void cleanupRender()
    {
        if (this->buffer != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(this->allocator, this->buffer, this->memory);
            this->buffer = VK_NULL_HANDLE;
        }
    }

    virtual ~Resources()
    {
        if (this->buffer != VK_NULL_HANDLE){
            cleanupRender();
        }
    }

    VmaAllocator allocator;
    VmaAllocation memory;
    vk::Buffer buffer = VK_NULL_HANDLE;
};
} // namespace star::StarBuffers