#pragma once

#include "Handle.hpp"
#include "SharedCompressedTexture.hpp"
#include "StarBuffers/Buffer.hpp"
#include "StarShader.hpp"
#include "TransferRequest_Texture.hpp"
#include "tasks/Task.hpp"
#include "renderer/RenderingTargetInfo.hpp"
#include "StarPipeline.hpp"

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

    CompressedTextureMetadata(const std::string &filePath, ktx_transcode_fmt_e targetFormat)
        : TextureMetadata(filePath), targetFormat(targetFormat)
    {
    }
};

struct CompileShaderPayload
{
    std::string path;
    star::Shader_Stage stage;
    uint32_t handleID;
    std::unique_ptr<StarShader> finalizedShaderObject = nullptr;
    std::unique_ptr<std::vector<uint32_t>> compiledShaderCode = nullptr;
};

struct IODataPreparationPayload
{
    std::variant<std::monostate, TextureMetadata, CompressedTextureMetadata> metadata;
    std::variant<std::monostate, std::unique_ptr<star::SharedCompressedTexture>> preparedData;
};

struct PipelineBuildPayload{
    vk::Device device; 
    uint32_t handleID; 
    std::unique_ptr<star::StarPipeline::RenderResourceDependencies> deps = nullptr; 
    std::unique_ptr<star::StarPipeline> pipeline = nullptr; 
};

namespace task_factory
{
star::job::tasks::Task<> createPrintTask(std::string message);

star::job::tasks::Task<> createImageWriteTask(std::string fileName, star::StarBuffers::Buffer imageBuffer);

star::job::tasks::Task<> createRecordCommandBufferTask(
    vk::CommandBuffer commandBuffer, std::function<void(vk::CommandBuffer &, const uint8_t &)> recordFunction,
    uint8_t frameInFlightIndex);

#pragma region BuildPipeline
void ExecuteBuildPipeline(void *p); 

std::optional<star::job::complete_tasks::CompleteTask<>> CreateBuildComplete(void *p); 

star::job::tasks::Task<> CreateBuildPipeline(vk::Device device, Handle handle, star::StarPipeline::RenderResourceDependencies buildDeps, StarPipeline pipeline);
#pragma endregion BuildPipeline

#pragma region CompileShader
// void DestroyCompilePayload(void *p);

// void MoveCompilePaylod(void *fresh, void *old);

void ExecuteCompileShader(void *p);

std::optional<star::job::complete_tasks::CompleteTask<>> CreateCompileComplete(void *p);

star::job::tasks::Task<> CreateCompileShader(const std::string &fileName, const star::Shader_Stage &stage,
                                             const Handle &shaderHandle);

#pragma endregion CompileShader

} // namespace task_factory
} // namespace star::job::tasks
