#pragma once

#include "job/complete_tasks/CompleteTask.hpp"
#include "job/tasks/Task.hpp"

#include "StarBuffers/Buffer.hpp"

#include <optional>

namespace star::job::tasks::write_image_to_disk
{

struct BufferImageInfo
{
    vk::Extent3D imageExtent;
    vk::Format imageFormat;
    StarBuffers::Buffer hostVisibleBufferImage;
};

struct WritePayload
{
    vk::Device device;
    std::unique_ptr<BufferImageInfo> bufferImageInfo = nullptr;
    std::unique_ptr<uint64_t> signalValue = nullptr;
    vk::Semaphore semaphore;
    std::string path;
};

using WriteImageTask = star::job::tasks::Task<sizeof(WritePayload), alignof(WritePayload)>;

std::optional<star::job::complete_tasks::CompleteTask> CreateComplete(void *p);

void Execute(void *p);

WriteImageTask Create(const vk::Device &device, const vk::Extent3D &imageExtent, const vk::Format &format,
                      StarBuffers::Buffer &buffer, const std::string &filePath, const uint64_t &semaphoreSignalValue,
                      const vk::Semaphore &signalSemaphore);

void WaitUntilSemaphoreIsReady(vk::Device &device, const vk::Semaphore &semaphore,
                               const uint64_t &signalValueToWaitFor);

} // namespace star::job::tasks::write_image_to_disk