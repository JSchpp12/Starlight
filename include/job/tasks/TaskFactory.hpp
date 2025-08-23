#pragma once

#include "StarBuffers/Buffer.hpp"
#include "tasks/Task.hpp"

#include <vulkan/vulkan.hpp>

#include <iostream>
#include <string>

namespace star::job::tasks
{
struct PrintPayload
{
    std::string message;
    PrintPayload() = default;
    PrintPayload(std::string message) : message(std::move(message))
    {
    }
    ~PrintPayload() = default;
};

struct CommandBufferRecordingPayload
{
    vk::CommandBuffer commandBuffer = vk::CommandBuffer();
    uint8_t frameInFlightIndex = 0; 
    std::function<void(vk::CommandBuffer &, const uint8_t &)> recordFunction = std::function<void(vk::CommandBuffer &, const uint8_t &)>(); 
};

struct CommandBufferSubmitPayload
{
    vk::CommandBuffer commandBuffer = vk::CommandBuffer(); 
    vk::PipelineStageFlags waitStages = vk::PipelineStageFlags(); 
    uint8_t frameInFlightIndex = 0; 
};

struct ImageWritePayload
{
    std::string fileName;
    VmaAllocator allocator;
    VmaAllocation memory;
    vk::DeviceSize bufferSize;
    vk::Buffer buffer;

    ImageWritePayload() = default;

    ImageWritePayload(std::string fileName, VmaAllocator allocator, VmaAllocation memory, vk::DeviceSize bufferSize,
                      vk::Buffer buffer)
        : fileName(std::move(fileName)), allocator(std::move(allocator)), memory(std::move(memory)),
          bufferSize(std::move(bufferSize)), buffer(std::move(buffer))
    {
    }

    ~ImageWritePayload() = default;
};

namespace task_factory
{

void imageWriteExecute(void *p);

void recordBufferExecute(void *p);

star::job::tasks::Task<> createPrintTask(std::string message);

star::job::tasks::Task<> createImageWriteTask(std::string fileName, star::StarBuffers::Buffer imageBuffer);

star::job::tasks::Task<> createRecordCommandBufferTask(
    vk::CommandBuffer commandBuffer, std::function<void(vk::CommandBuffer &, const uint8_t &)> recordFunction, uint8_t frameInFlightIndex);

// star::job::Task<> createIm

} // namespace task_factory
} // namespace star::job
