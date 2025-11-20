#include "job/tasks/WriteImageToDisk.hpp"

#include "FileHelpers.hpp"
#include "logging/LoggingFactory.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
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
    WriteImageToDisk(payload->bufferImageInfo->hostVisibleBufferImage, *payload->bufferImageInfo, payload->path);

    core::logging::log(boost::log::trivial::info, "PNG Write Done"); 
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

bool WriteImageToDisk(StarBuffers::Buffer &buffer, BufferImageInfo &info, std::string &path)
{
    const uint32_t width = info.imageExtent.width;
    const uint32_t height = info.imageExtent.height;

    void *mapped = nullptr;
    info.hostVisibleBufferImage.map(&mapped); // Or .data() depending on your wrapper
    if (!mapped)
    {
        throw std::runtime_error("Failed to map buffer for image write.");
    }

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
        info.hostVisibleBufferImage.unmap();
        throw std::runtime_error("Unsupported image format for PNG writing.");
    }

    const uint32_t rowStride = width * comp; // tightly packed

    // ---- PNG write ----------------------------------------------------------
    int ok = stbi_write_png(path.c_str(), width, height, comp, mapped, rowStride);

    info.hostVisibleBufferImage.unmap();
    return ok != 0;
}
} // namespace star::job::tasks::write_image_to_disk