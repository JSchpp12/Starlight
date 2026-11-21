#pragma once

#include "core/graphics/GPUWorkSyncInfo.hpp"
#include "data_structure/dynamic/ThreadSharedObjectPool.hpp"
#include "job/tasks/Task.hpp"
#include "job/tasks/actions/WritePngImageAction.hpp"
#include "job/tasks/actions/WritePngMaskAction.hpp"
#include "job/tasks/actions/WriteTiffImageAction.hpp"
#include "starlight/maker/buffer/MakeBuffer.hpp"

#include "StarBuffers/Buffer.hpp"

#include <boost/atomic/atomic.hpp>

#include <star_common/HandleTypeRegistry.hpp>

#include <optional>

namespace star::job::tasks::write_image_to_disk
{

inline static constexpr std::string_view WriteImageTypeName = "star::job::tasks::write_image_to_disk";

/// @brief Registry id of the image-writing task pool, resolved once and cached.
inline uint16_t GetWriteImageTaskTypeTag()
{
    static const uint16_t id = star::common::HandleTypeRegistry::instance().registerType(WriteImageTypeName);
    return id;
}

struct PoolOwnedWriteImagePayload
{
    using PoolType =
        data_structure::dynamic::ThreadSharedObjectPool<star::StarBuffers::Buffer, star::maker::buffer::MakeBuffer, 50>;

    struct Data
    {
        std::string path;
        vk::Extent3D imageExtent;
        vk::Format imageFormat;
        vk::Device device{VK_NULL_HANDLE};
        std::optional<star::core::graphics::SemaphoreSyncPoint> waitInfo;
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
        std::optional<star::core::graphics::SemaphoreSyncPoint> waitInfo;
    };

    std::unique_ptr<Data> data;
    StarBuffers::Buffer *buffer = nullptr;

    void operator()();
};

using WriteImageTask = star::job::tasks::Task<>;

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
        .setTypeTag(GetWriteImageTaskTypeTag())
        .build();
}

void WaitUntilSemaphoreIsReady(vk::Device &device, const vk::Semaphore &semaphore,
                               const uint64_t &signalValueToWaitFor);

} // namespace star::job::tasks::write_image_to_disk