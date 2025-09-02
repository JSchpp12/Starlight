#include "tasks/TaskFactory.hpp"

#include "complete_tasks/CompleteTask.hpp"
#include "complete_tasks/TaskFactory.hpp"

#include "FileHelpers.hpp"

#include <memory>

// void star::job::tasks::task_factory::textureIOExecute(IODataPreparationPayload *data)
// {
//     assert(data != nullptr);

//     if (std::holds_alternative<CompressedTextureMetadata>(data->metadata))
//     {
//         auto &compMeta = std::get<CompressedTextureMetadata>(data->metadata);
//         auto compTex = std::make_unique<SharedCompressedTexture>(compMeta.filePath, compMeta.targetFormat);
//         compTex->triggerTranscode();
//         data->preparedData = std::move(compTex);
//     }
//     else if (std::holds_alternative<TextureMetadata>(data->metadata))
//     {
//         throw std::runtime_error("Cant do regular textures yet");
//     }
//     else
//     {
//         throw std::runtime_error("Failed to parse variant of payload metadata for texture");
//     }
// }

// void star::job::tasks::task_factory::ioDataExecute(void *p)
// {
//     auto *data = static_cast<IODataPreparationPayload *>(p);

//     if (std::holds_alternative<CompressedTextureMetadata>(data->metadata) ||
//         std::holds_alternative<TextureMetadata>(data->metadata))
//     {
//         textureIOExecute(data);
//     }
//     else
//     {
//         throw std::runtime_error("Invalid transfer kind");
//     }
// }

// std::optional<star::job::complete_tasks::CompleteTask<>> star::job::tasks::task_factory::ioDataCreateComplete(void *p)
// {
//     auto *data = static_cast<IODataPreparationPayload *>(p);

//     if (std::holds_alternative<CompressedTextureMetadata>(data->metadata))
//     {
//         auto &metadata = std::get<CompressedTextureMetadata>(data->metadata);
//         auto &compData = std::get<std::unique_ptr<star::SharedCompressedTexture>>(data->preparedData);
//         return std::optional<complete_tasks::CompleteTask<>>(
//             complete_tasks::task_factory::createTextureTransferComplete(metadata.gpuResourceReady,
//                                                                         std::move(compData)));
//     }
//     else if (std::holds_alternative<TextureMetadata>(data->metadata))
//     {
//     }
//     else
//     {
//         throw std::runtime_error("Invalid metadatatype");
//     }

//     return std::nullopt;
// }

// void star::job::tasks::task_factory::imageWriteExecute(void *p)
// {
// }

// star::job::tasks::Task<> star::job::tasks::task_factory::createTextureTransferTask(
//     const std::string &filePath, const vk::PhysicalDevice &physicalDevice, vk::Semaphore gpuResourceReady)
// {
//     ktx_transcode_fmt_e targetFormat = star::SharedCompressedTexture::GetResultTargetCompressedFormat(physicalDevice);

//     IODataPreparationPayload payload{.metadata = CompressedTextureMetadata{
//                                          filePath,
//                                          gpuResourceReady,
//                                          targetFormat,
//                                      }};

//     return star::job::tasks::Task<>::Builder<IODataPreparationPayload>()
//         .setPayload(std::move(payload))
//         .setExecute(&ioDataExecute)
//         .setCreateCompleteTaskFunction(&ioDataCreateComplete)
//         .build();
// }

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

star::job::tasks::Task<> star::job::tasks::task_factory::createImageWriteTask(std::string fileName,
                                                                              StarBuffers::Buffer imageBuffer)
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
    vk::CommandBuffer commandBuffer, std::function<void(vk::CommandBuffer &, const uint8_t &)> recordFunction,
    uint8_t frameInFlightIndex)
{
    return star::job::tasks::Task<>::Builder<CommandBufferRecordingPayload>()
        .setPayload(CommandBufferRecordingPayload{
            commandBuffer,
            frameInFlightIndex,
            recordFunction,
        })
        .setExecute([](void *p) {
            auto *payload = static_cast<CommandBufferRecordingPayload *>(p);
            payload->recordFunction(payload->commandBuffer, payload->frameInFlightIndex);
        })
        .build();
}

// star::job::tasks::Task<>
// star::job::tasks::task_factory::createTextureTransferTask(std::unique_ptr<SharedCompressedTexture> compressedTex)
// {
//     return star::job::tasks::Task<>();
// }
