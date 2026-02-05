#pragma once

#include "data_structure/dynamic/ThreadSharedObjectPool.hpp"
#include "job/tasks/Task.hpp"
#include "wrappers/graphics/policies/GenericBufferCreateAllocatePolicy.hpp"

#include "StarBuffers/Buffer.hpp"

#include <boost/atomic/atomic.hpp>

#include <optional>

namespace star::job::tasks::write_image_to_disk
{

inline static constexpr std::string_view WriteImageTypeName = "star::job::tasks::write_image_to_disk";

struct BufferImageInfo
{
    Handle registrationHandle;
    vk::Extent3D imageExtent;
    vk::Format imageFormat;
    data_structure::dynamic::ThreadSharedObjectPool<star::StarBuffers::Buffer,
                                                    wrappers::graphics::policies::GenericBufferCreateAllocatePolicy,
                                                    500> *owningObjectPool = nullptr;
};

struct WritePayload
{
    std::string path;
    vk::Semaphore semaphore;
    vk::Device device;
    std::unique_ptr<BufferImageInfo> bufferImageInfo = nullptr;
    std::unique_ptr<uint64_t> signalValue = nullptr;
};

using WriteImageTask = star::job::tasks::Task<sizeof(WritePayload), alignof(WritePayload)>;

std::optional<star::job::complete_tasks::CompleteTask> CreateComplete(void *p);

void Execute(void *p);

WriteImageTask Create(WritePayload payload);

void WaitUntilSemaphoreIsReady(vk::Device &device, const vk::Semaphore &semaphore,
                               const uint64_t &signalValueToWaitFor);

bool WriteImageToDisk(StarBuffers::Buffer &buffer, BufferImageInfo &info, std::string &path);

} // namespace star::job::tasks::write_image_to_disk