#pragma once

#include "CompleteTask.hpp"
#include "SharedCompressedTexture.hpp"

#include <vulkan/vulkan.hpp>
namespace star::job::complete_tasks
{
struct SuccessPayload
{
    bool wasSuccessful;
};

struct TextureTransferCompletePayload{
    vk::Semaphore gpuResourceReady; 
};

struct CompressedTextureTransferCompletePaylod : public TextureTransferCompletePayload{
    std::unique_ptr<star::SharedCompressedTexture> texture;
};

namespace task_factory
{
star::job::complete_tasks::CompleteTask<> createStandardSuccess();

void textureTransferCompleteExecute(core::device::StarDevice *device, void *payload);

star::job::complete_tasks::CompleteTask<> createTextureTransferComplete(vk::Semaphore gpuResourceReady, std::unique_ptr<SharedCompressedTexture> compressedTex); 

}
}; // namespace star::job::complete_tasks