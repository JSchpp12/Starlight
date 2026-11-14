#include "job/tasks/WriteImageToDisk.hpp"

#include "FileHelpers.hpp"
#include "logging/LoggingFactory.hpp"

#include "job/tasks/actions/WritePngImageAction.hpp"
#include "starlight/core/Exceptions.hpp"

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

static void LogImageInfo(vk::Format format, const vk::Extent3D &extent) noexcept
{
    std::ostringstream oss;
    oss << "Info for image: " << std::endl
        << "GPU Format: " << vk::to_string(format) << std::endl
        << "GPU Extent: " << extent.width << ", " << extent.height << std::endl;
    star::core::logging::debug(oss.str());
}

void PoolOwnedWriteImagePayload::operator()()
{
    if (data == nullptr)
    {
        STAR_THROW("Write payload data is invalid");
    }

    LogStart(data->path);

    if (data->waitInfo.has_value())
    {
        star::core::logging::debug(data->path + " - Starting wait for semaphore. Value: " +
                                   std::to_string(data->waitInfo.value().signalValue));
        WaitUntilSemaphoreIsReady(data->device, data->waitInfo.value().vkSemaphore, data->waitInfo->signalValue);
        star::core::logging::debug(data->path + " - Done waiting for semaphore");
    }

    LogImageInfo(data->imageFormat, data->imageExtent);
    auto &buffer = data->owningObjectPool->get(data->registrationHandle);
    if (actions::IsPngFormat(data->imageFormat))
    {
        star::core::logging::debug("Processing image as PNG regular image");
        actions::WritePngImageAction{data->imageExtent, data->imageFormat, data->path,
                                     actions::VulkanBufferSource{buffer}}();
    }
    else if (actions::IsPngMaskFormat(data->imageFormat))
    {
        star::core::logging::debug("Writing image as PNG mask");
        actions::WritePngMaskAction{data->imageExtent, data->imageFormat, data->path,
                                    actions::VulkanBufferSource{buffer}}();
    }
    else if (actions::IsTiffFormat(data->imageFormat))
    {
        star::core::logging::debug("Writing image as TIFF");
        actions::WriteTiffImageAction{data->imageExtent, data->imageFormat, data->path,
                                      actions::VulkanBufferSource{buffer}}();
    }
    else
    {
        data->owningObjectPool->release(data->registrationHandle);
        throw std::runtime_error("Unsupported image format for image writing.");
    }
    LogDone(data->path);
    data->owningObjectPool->release(data->registrationHandle);
}

void DirectWriteImagePayload::operator()()
{
    if (data == nullptr)
    {
        STAR_THROW("Write payload data is invalid");
    }

    LogStart(data->path);

    if (data->waitInfo.has_value())
    {
        star::core::logging::info(data->path + " - Starting wait for semaphore. Value: " +
                                  std::to_string(data->waitInfo.value().signalValue));
        WaitUntilSemaphoreIsReady(data->device, data->waitInfo.value().vkSemaphore, data->waitInfo->signalValue);
        star::core::logging::info(data->path + " - Done waiting for semaphore");
    }

    if (actions::IsPngFormat(data->imageFormat))
    {
        actions::WritePngImageAction{data->imageExtent, data->imageFormat, data->path,
                                     actions::VulkanBufferSource{*buffer}}();
    }
    else if (actions::IsPngMaskFormat(data->imageFormat))
    {
        actions::WritePngMaskAction{data->imageExtent, data->imageFormat, data->path,
                                    actions::VulkanBufferSource{*buffer}}();
    }
    else if (actions::IsTiffFormat(data->imageFormat))
    {
        actions::WriteTiffImageAction{data->imageExtent, data->imageFormat, data->path,
                                      actions::VulkanBufferSource{*buffer}}();
    }
    else
    {
        throw std::runtime_error("Unsupported image format for image writing.");
    }
    LogDone(data->path);
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
} // namespace star::job::tasks::write_image_to_disk