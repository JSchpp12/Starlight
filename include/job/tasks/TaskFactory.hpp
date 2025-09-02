#pragma once

#include "SharedCompressedTexture.hpp"
#include "StarBuffers/Buffer.hpp"
#include "tasks/Task.hpp"
#include "TransferRequest_Texture.hpp"

#include <vulkan/vulkan.hpp>

#include <ktx.h>

#include <iostream>
#include <string>
#include <variant>

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
    std::function<void(vk::CommandBuffer &, const uint8_t &)> recordFunction =
        std::function<void(vk::CommandBuffer &, const uint8_t &)>();
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

enum class TransferKind
{
    None,
    Texture,
    Buffer
};

enum class TextureKind
{
    Compressed,
    Raw
};

struct TextureMetadata
{
    std::string filePath = "";
};

struct CompressedTextureMetadata : public TextureMetadata
{
    ktx_transcode_fmt_e targetFormat;

    CompressedTextureMetadata(const std::string &filePath,
                              ktx_transcode_fmt_e targetFormat)
        : TextureMetadata(filePath), targetFormat(targetFormat)
    {
    }
};

struct IODataPreparationPayload
{
    std::variant<std::monostate, TextureMetadata, CompressedTextureMetadata> metadata;
    std::variant<std::monostate, std::unique_ptr<star::SharedCompressedTexture>> preparedData;
};

namespace task_factory
{

void imageWriteExecute(void *p);

void recordBufferExecute(void *p);

// void textureIOExecute(IODataPreparationPayload *data);

// void ioDataExecute(void *p);

// std::optional<complete_tasks::CompleteTask<>> ioDataCreateComplete(void *p);

// star::job::tasks::Task<> createTextureTransferTask(const std::string &filePath,
//                                                    const vk::PhysicalDevice &physicalDevice,
//                                                    vk::Semaphore gpuResourceReady);

star::job::tasks::Task<> createPrintTask(std::string message);

star::job::tasks::Task<> createImageWriteTask(std::string fileName, star::StarBuffers::Buffer imageBuffer);

star::job::tasks::Task<> createRecordCommandBufferTask(
    vk::CommandBuffer commandBuffer, std::function<void(vk::CommandBuffer &, const uint8_t &)> recordFunction,
    uint8_t frameInFlightIndex);

// star::job::tasks::Task<> createTextureTransferTask(std::unique_ptr<SharedCompressedTexture> compressedTex);

} // namespace task_factory
} // namespace star::job::tasks
