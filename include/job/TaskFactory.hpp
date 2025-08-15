#pragma once

#include "StarBuffers/Buffer.hpp"
#include "Task.hpp"

#include <iostream>
#include <string>

namespace star::job
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

namespace TaskFactory
{
void printExecute(void *p);

void imageWriteExecute(void *p);

star::job::Task<> createPrintTask(std::string message);

star::job::Task<> createImageWriteTask(std::string fileName, star::StarBuffers::Buffer imageBuffer);

} // namespace TaskFactory
} // namespace star::job
