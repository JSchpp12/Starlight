#include "job/tasks/WriteImageToDisk.hpp"

#include "FileHelpers.hpp"
#include "logging/LoggingFactory.hpp"

#include "starlight/core/Exceptions.hpp"
#include <star_common/helper/CastHelpers.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
namespace star::job::tasks::write_image_to_disk
{

static void LogStart(const std::string &fileName)
{
    core::logging::info("Start write - " + fileName);
}

static void LogDone(const std::string &fileName)
{
    core::logging::info("Done write - " + fileName);
}

std::optional<star::job::complete_tasks::CompleteTask> CreateComplete(void *p)
{
    return std::nullopt;
}

void Execute(void *p)
{
    auto *payload = static_cast<WritePayload *>(p);

    // if (auto parentDir = file_helpers::GetParentDirectory(payload->path))
    // {
    //     if (!boost::filesystem::exists(parentDir.value()))
    //     {
    //         file_helpers::CreateDirectoryIfDoesNotExist(parentDir.value());
    //     }
    // }

    if (payload->addData == nullptr)
    {
        STAR_THROW("Additional data is invalid");
    }
    LogStart(payload->path);

    star::core::logging::info(
        payload->path + " - Starting wait for semaphore. Value: " + std::to_string(payload->addData->signalValue));
    WaitUntilSemaphoreIsReady(payload->device, payload->semaphore, payload->addData->signalValue);
    star::core::logging::info(payload->path + " - Done waiting for semaphore");
    WriteImageToDisk(
        payload->addData->bufferImageInfo.owningObjectPool->get(payload->addData->bufferImageInfo.registrationHandle),
        payload->addData->bufferImageInfo, payload->path);
    LogDone(payload->path);
    payload->addData->bufferImageInfo.owningObjectPool->release(payload->addData->bufferImageInfo.registrationHandle);
}

WriteImageTask Create(WritePayload payload)
{
    return WriteImageTask::Builder<WritePayload>()
        .setPayload(std::move(payload))
        .setCreateCompleteTaskFunction(&CreateComplete)
        .setExecute(&Execute)
        .build();
}

void WaitUntilSemaphoreIsReady(vk::Device &device, const vk::Semaphore &semaphore, const uint64_t &signalValueToWaitFor)
{
    vk::Result waitResult = device.waitSemaphores(
        vk::SemaphoreWaitInfo().setValues(signalValueToWaitFor).setSemaphores(semaphore), UINT64_MAX);

    if (waitResult != vk::Result::eSuccess)
    {
        STAR_THROW("Failed to wait for semaphore. Copy taking too long");
    }
}

bool WriteImageToDisk(StarBuffers::Buffer &buffer, BufferImageInfo &info, std::string &path)
{
    const uint32_t width = info.imageExtent.width;
    const uint32_t height = info.imageExtent.height;

    void *mapped = nullptr;
    buffer.map(&mapped); // Or .data() depending on your wrapper
    if (!mapped)
    {
        STAR_THROW("Failed to map buffer for image write");
    }
    const auto result = buffer.invalidate();

    // ---- Format handling ----------------------------------------------------
    int comp = 4; // RGBA
    vk::Format fmt = info.imageFormat;

    if (fmt == vk::Format::eR8G8B8A8Unorm || fmt == vk::Format::eR8G8B8A8Srgb || fmt == vk::Format::eB8G8R8A8Unorm ||
        fmt == vk::Format::eB8G8R8A8Srgb)
    {
        // ok
    }
    else
    {
        // You can extend support by converting here.
        buffer.unmap();
        throw std::runtime_error("Unsupported image format for PNG writing.");
    }

    int w = 0;
    star::common::casts::SafeCast(width, w);
    int h = 0;
    star::common::casts::SafeCast(height, h);
    const int rowStride = w * comp; // tightly packed

    // ---- PNG write ----------------------------------------------------------
    int ok = stbi_write_png(path.c_str(), w, h, comp, mapped, rowStride);

    buffer.unmap();
    return ok != 0;
}
} // namespace star::job::tasks::write_image_to_disk