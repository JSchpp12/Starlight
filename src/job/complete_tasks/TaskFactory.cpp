#include "complete_tasks/TaskFactory.hpp"

#include "device/StarDevice.hpp"
#include "job/tasks/TaskFactory.hpp"


star::job::complete_tasks::CompleteTask<> star::job::complete_tasks::task_factory::createStandardSuccess(){
    return complete_tasks::CompleteTask<>::Builder<SuccessPayload>()
        .setPayload(SuccessPayload())
        .setEngineExecuteFunction([](star::core::device::StarDevice *context, void *p){
            auto *payload = static_cast<SuccessPayload *>(p);
        })
        .build();
}

void star::job::complete_tasks::task_factory::textureTransferCompleteExecute(core::device::StarDevice *device, void *payload)
{
    std::cout << "test" << std::endl; 
}

star::job::complete_tasks::CompleteTask<> star::job::complete_tasks::task_factory::createTextureTransferComplete(vk::Semaphore gpuResourceReady, std::unique_ptr<SharedCompressedTexture> compressedTex){
    return complete_tasks::CompleteTask<>::Builder<CompressedTextureTransferCompletePaylod>()
        .setPayload(CompressedTextureTransferCompletePaylod{
            .texture = std::move(compressedTex)
        })
        .setEngineExecuteFunction(&textureTransferCompleteExecute)
        .build();
}