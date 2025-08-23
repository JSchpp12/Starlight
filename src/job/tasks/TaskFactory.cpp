#include "tasks/TaskFactory.hpp"

void star::job::tasks::task_factory::imageWriteExecute(void *p)
{
}

star::job::tasks::Task<> star::job::tasks::task_factory::createPrintTask(std::string message)
{
    return star::job::tasks::Task<>::Builder<PrintPayload>()
        .setPayload(PrintPayload(std::move(message)))
        .setExecute([](void *p) {
            auto *data = static_cast<PrintPayload *>(p);
            std::cout << data->message << std::endl;
        })
        .build();
}

star::job::tasks::Task<> star::job::tasks::task_factory::createImageWriteTask(std::string fileName, StarBuffers::Buffer imageBuffer)
{
    auto resources = imageBuffer.releaseResources();

    auto task = star::job::tasks::Task<>::Builder<ImageWritePayload>()
                    .setPayload(ImageWritePayload(fileName.c_str(), resources->allocator, resources->memory,
                                                  imageBuffer.getBufferSize(), resources->buffer))
                    .setExecute(&task_factory::imageWriteExecute)
                    .build();

    resources->buffer = VK_NULL_HANDLE;
    return task;
}

star::job::tasks::Task<> star::job::tasks::task_factory::createRecordCommandBufferTask(
    vk::CommandBuffer commandBuffer, std::function<void(vk::CommandBuffer &, const uint8_t &)> recordFunction, uint8_t frameInFlightIndex)
{
    return star::job::tasks::Task<>::Builder<CommandBufferRecordingPayload>().setPayload(
        CommandBufferRecordingPayload{
            commandBuffer, 
            frameInFlightIndex,
            recordFunction, 
        })
        .setExecute([](void *p){
            auto *payload = static_cast<CommandBufferRecordingPayload *>(p);
            payload->recordFunction(payload->commandBuffer, payload->frameInFlightIndex);
        })
        .build();
}
