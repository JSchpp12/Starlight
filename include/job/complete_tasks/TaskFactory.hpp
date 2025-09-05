#pragma once

#include "CompleteTask.hpp"
#include "SharedCompressedTexture.hpp"
#include "StarShader.hpp"

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
    size_t handleID;
    std::unique_ptr<star::StarShader> finalizedShaderObject = nullptr;
    std::unique_ptr<std::vector<uint32_t>> compiledShaderCode = nullptr; 
};

struct CompressedTextureTransferCompletePaylod : public TextureTransferCompletePayload
{
    std::unique_ptr<star::SharedCompressedTexture> texture;
};

namespace task_factory
{

#pragma region CompileShaders

// void MoveShaderCompletePaylod(void *fresh, void *old);

// void DestroyShaderCompletePayload(void *object);

void ExecuteShaderCompileComplete(void *device, void *shaderManager, void *payload);

star::job::complete_tasks::CompleteTask<> CreateShaderCompileComplete(size_t handleID, std::unique_ptr<StarShader> finalizedShaderObject,
                                                                      std::unique_ptr<std::vector<uint32_t>> finalizedCompiledShader);

#pragma endregion CompileShaders
} // namespace task_factory
}; // namespace star::job::complete_tasks