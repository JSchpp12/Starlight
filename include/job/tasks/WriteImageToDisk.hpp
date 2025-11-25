#pragma once

#include "job/tasks/Task.hpp"

#include "StarBuffers/Buffer.hpp"

#include <boost/atomic/atomic.hpp>

#include <optional>

namespace star::job::tasks::write_image_to_disk
{

constexpr std::string_view WriteImageTypeName = "star::job::tasks::write_image_to_disk";

struct BufferImageInfo
{
    vk::Extent3D imageExtent;
    vk::Format imageFormat;
    StarBuffers::Buffer hostVisibleBufferImage;
};

struct WritePayload
{
    std::string path;
    vk::Semaphore semaphore;
    vk::Device device;
    std::unique_ptr<BufferImageInfo> bufferImageInfo = nullptr;
    std::unique_ptr<uint64_t> signalValue = nullptr;
    boost::atomic<bool> *writeIsDoneFlag = nullptr;
};

using WriteImageTask = star::job::tasks::Task<sizeof(WritePayload), alignof(WritePayload)>;

std::optional<star::job::complete_tasks::CompleteTask> CreateComplete(void *p);

void Execute(void *p);

WriteImageTask Create(boost::atomic<bool> *writeIsDoneFlag, const vk::Device &device, const vk::Extent3D &imageExtent,
                      const vk::Format &format, StarBuffers::Buffer &buffer, const std::string &filePath,
                      const uint64_t &semaphoreSignalValue, const vk::Semaphore &signalSemaphore);

void WaitUntilSemaphoreIsReady(vk::Device &device, const vk::Semaphore &semaphore,
                               const uint64_t &signalValueToWaitFor);

bool WriteImageToDisk(StarBuffers::Buffer &buffer, BufferImageInfo &info, std::string &path);

} // namespace star::job::tasks::write_image_to_disk