#include "job/tasks/WriteImageToDisk.hpp"

#include "FileHelpers.hpp"
#include "logging/LoggingFactory.hpp"

namespace star::job::tasks::write_image_to_disk
{

std::optional<star::job::complete_tasks::CompleteTask> CreateComplete(void *p)
{
    return std::nullopt;
}

void Execute(void *p)
{
    core::logging::log(boost::log::trivial::info, "Beginning image write");

    auto *payload = static_cast<WritePayload *>(p);

    // if (auto parentDir = file_helpers::GetParentDirectory(payload->path))
    // {
    //     if (!boost::filesystem::exists(parentDir.value()))
    //     {
    //         file_helpers::CreateDirectoryIfDoesNotExist(parentDir.value());
    //     }
    // }

    WaitUntilSemaphoreIsReady(payload->device, payload->semaphore, *payload->signalValue);

    core::logging::log(boost::log::trivial::info, "Signal semaphore is done");
}

WriteImageTask Create(const vk::Device &device, const vk::Extent3D &imageExtent, const vk::Format &format,
                      StarBuffers::Buffer &buffer, const std::string &filePath, const uint64_t &semaphoreSignalValue,
                      const vk::Semaphore &signalSemaphore)
{
    return WriteImageTask::Builder<WritePayload>()
        .setPayload(WritePayload{.device = device,
                                 .bufferImageInfo = std::make_unique<BufferImageInfo>(
                                     vk::Extent3D{imageExtent}, vk::Format{format}, StarBuffers::Buffer{buffer}),
                                 .signalValue = std::make_unique<uint64_t>(semaphoreSignalValue),
                                 .semaphore = signalSemaphore,
                                 .path = filePath})
        .setCreateCompleteTaskFunction(&CreateComplete)
        .setExecute(&Execute)
        .build();
}

void WaitUntilSemaphoreIsReady(vk::Device &device, const vk::Semaphore &semaphore, const uint64_t &signalValueToWaitFor)
{
    vk::Result waitResult = device.waitSemaphores(
        vk::SemaphoreWaitInfo().setSemaphoreCount(1).setPSemaphores(&semaphore).setPValues(&signalValueToWaitFor),
        UINT64_MAX);

    if (waitResult != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to wait for semaphore. Copy taking too long");
    }
}

void WriteImageToDisk(StarBuffers::Buffer &buffer)
{
    
}
} // namespace star::job::tasks::write_image_to_disk