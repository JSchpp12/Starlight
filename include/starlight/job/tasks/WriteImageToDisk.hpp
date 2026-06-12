#pragma once

#include "data_structure/dynamic/ThreadSharedObjectPool.hpp"
#include "job/tasks/Task.hpp"
#include "job/tasks/actions/WritePngImageAction.hpp"
#include "job/tasks/actions/WriteTiffImageAction.hpp"
#include "starlight/wrappers/graphics/policies/GenericBufferCreateAllocatePolicy.hpp"
#include "starlight/wrappers/graphics/StarSemaphore.hpp"

#include "StarBuffers/Buffer.hpp"

#include <boost/atomic/atomic.hpp>

#include <optional>

namespace star::job::tasks::write_image_to_disk
{

inline static constexpr std::string_view WriteImageTypeName = "star::job::tasks::write_image_to_disk";

struct PoolOwnedWriteImagePayload
{
    using PoolType = data_structure::dynamic::ThreadSharedObjectPool<star::StarBuffers::Buffer,
                                                                     wrappers::graphics::policies::GenericBufferCreateAllocatePolicy,
                                                                     50>;

    struct Data
    {
        std::string path;
        vk::Extent3D imageExtent;
        vk::Format imageFormat;
        vk::Device device{VK_NULL_HANDLE};
        std::optional<star::StarSemaphore> waitInfo;
        Handle registrationHandle;
        PoolType *owningObjectPool = nullptr;
    };

    std::unique_ptr<Data> data;

    void operator()();
};

struct DirectWriteImagePayload
{
    struct Data
    {
        std::string path;
        vk::Extent3D imageExtent;
        vk::Format imageFormat;
        vk::Device device{VK_NULL_HANDLE};
        std::optional<star::StarSemaphore> waitInfo;
    };

    std::unique_ptr<Data> data;
    StarBuffers::Buffer *buffer = nullptr;

    void operator()();
};

using WriteImageTask = star::job::tasks::Task<128, alignof(std::max_align_t)>;

std::optional<star::job::complete_tasks::CompleteTask> CreateComplete(void *p);

template <typename T> void Execute(void *p)
{
    auto *payload = static_cast<T *>(p);
    payload->operator()();
}

template <typename T> WriteImageTask Create(T payload)
{
    return WriteImageTask::Builder<T>()
        .setPayload(std::move(payload))
        .setCreateCompleteTaskFunction(&CreateComplete)
        .setExecute(&Execute<T>)
        .build();
}

void WaitUntilSemaphoreIsReady(vk::Device &device, const vk::Semaphore &semaphore,
                                const uint64_t &signalValueToWaitFor);

} // namespace star::job::tasks::write_image_to_disk