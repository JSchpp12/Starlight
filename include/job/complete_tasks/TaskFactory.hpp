#pragma once

#include "SharedCompressedTexture.hpp"
#include "StarPipeline.hpp"
#include "StarShader.hpp"
#include "StarTextures/Texture.hpp"
#include "job/tasks/task_factory/CompileShader.hpp"

#include <vulkan/vulkan.hpp>
namespace star::job::complete_tasks
{
struct SuccessPayload
{
    bool wasSuccessful;
};

struct TextureTransferCompletePayload
{
    vk::Semaphore gpuResourceReady;
};

struct CompileCompletePayload
{
    uint32_t handleID;
    std::unique_ptr<star::StarShader> finalizedShaderObject = nullptr;
    std::unique_ptr<std::vector<uint32_t>> compiledShaderCode = nullptr;
};

struct PipelineBuildCompletePayload
{
    uint32_t handleID = 0;
    std::unique_ptr<star::StarPipeline> pipeline = nullptr;
};

struct CompressedTextureTransferCompletePayload : public TextureTransferCompletePayload
{
    std::unique_ptr<star::SharedCompressedTexture> texture;
};

struct ImageWriteToDiskCompletePayload{
    std::unique_ptr<StarTextures::Texture> texture = nullptr; 
};

namespace task_factory
{

#pragma region BuildPipeline

void ExecuteBuildPipelineComplete(void *device, void *taskSystem, void *eventBus, void *graphicsManagers,
                                  void *payload);

complete_tasks::CompleteTask CreateBuildPipelineComplete(uint32_t handleID, std::unique_ptr<StarPipeline> pipeline);

#pragma endregion BuildPipeline

#pragma region CompileShaders

void ProcessPipelinesWhichAreNowReadyForBuild(void *device, void *taskSystem, void *graphicsManagers);

void ExecuteShaderCompileComplete(void *device, void *taskSystem, void *eventBus, void *graphicsManagers,
                                  void *payload);

star::job::complete_tasks::CompleteTask CreateShaderCompileComplete(
    uint32_t handleID, std::unique_ptr<StarShader> finalizedShaderObject,
    std::unique_ptr<std::vector<uint32_t>> finalizedCompiledShader);

} // namespace task_factory
}; // namespace star::job::complete_tasks