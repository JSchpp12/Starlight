#pragma once

#include "StarBuffer.hpp"
#include "StarDevice.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>

namespace star::TransferRequest{
    ///Request object used by secondary thread manager to copy resources to GPU. Templated to allow for different types of creation args such as BufferCreationArgs.
    template <typename T>
    class Memory {
    public:

    Memory() = default;
    ~Memory() = default; 

    virtual std::unique_ptr<StarBuffer> createStagingBuffer(vk::Device& device, VmaAllocator& allocator, const uint32_t& transferQueueFamilyIndex) const = 0; 

    virtual std::unique_ptr<T> createFinal(vk::Device& device, VmaAllocator& allocator, const uint32_t& transferQueueFamilyIndex) const = 0; 

    virtual void copyFromTransferSRCToDST(StarBuffer& srcBuffer, T& dst, vk::CommandBuffer& commandBuffer) const = 0; 
    
    virtual void writeDataToStageBuffer(StarBuffer& buffer) const = 0; 

    protected:

    };
}
