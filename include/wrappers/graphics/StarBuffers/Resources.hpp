#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace star::StarBuffers
{
class Resources
{
  public:
    Resources(VmaAllocator &allocator, VmaAllocation &memory, vk::Buffer &buffer)
        : allocator(allocator), memory(memory), buffer(buffer)
    {
    }

    ~Resources(){
        vmaDestroyBuffer(this->allocator, this->buffer, this->memory);
    }

    VmaAllocator allocator;
    VmaAllocation memory;
    vk::Buffer buffer   = VK_NULL_HANDLE;

    private:

};
} // namespace star::StarBuffers