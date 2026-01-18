#pragma once

#include "SharedCompressedTexture.hpp"
#include "StarPipeline.hpp"
#include "StarShader.hpp"
#include "StarTextures/Texture.hpp"
#include "job/tasks/CompileShader.hpp"

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

} // namespace task_factory
}; // namespace star::job::complete_tasks