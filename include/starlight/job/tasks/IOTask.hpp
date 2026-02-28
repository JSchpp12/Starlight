#pragma once

#include "job/tasks/Task.hpp"

#include <functional>
#include <future>
#include <string_view>

namespace star::job::tasks::io
{
inline static constexpr std::string_view IOTaskName = "star::job::tasks::io";

template <class TFn> struct WritePayload
{
    std::string filePath;
    TFn writeFunction;

    WritePayload() = default;
    WritePayload(std::string filePath, TFn writeFunction)
        : filePath(std::move(filePath)), writeFunction(std::move(writeFunction))
    {
    }

    int operator()()
    {
        star::core::logging::info("Starting file write: " + filePath);
        auto result = std::invoke(writeFunction, filePath);
        star::core::logging::info("Done writing file: " + filePath);

        return result;
    }
};

template <class TFn> struct ReadPayload
{
    std::string filePath;
    TFn readFunction;

    ReadPayload() = default;
    ReadPayload(std::string filePath, TFn readFunction)
        : filePath(std::move(filePath)), readFunction(std::move(readFunction))
    {
    }

    int operator()()
    {
        star::core::logging::info("Starting file read: " + filePath);
        auto result = std::invoke(readFunction, filePath);
        star::core::logging::info("Done reading file: " + filePath);

        return result;
    }
};

using IOTask = star::job::tasks::Task<128, alignof(std::max_align_t)>;

std::optional<star::job::complete_tasks::CompleteTask> CreateWriteTaskComplete(void *p);

template <class TFn> void ExecuteWriteTask(void *p)
{
    auto *payload = static_cast<WritePayload<TFn> *>(p);
    payload->operator()();
}

template <class TFn> void ExecuteReadTask(void *p)
{
    auto *payload = static_cast<ReadPayload<TFn> *>(p);
    payload->operator()();
}

template <class TFn> IOTask CreateWriteTask(WritePayload<TFn> writePayload)
{
    return IOTask::Builder<WritePayload<TFn>>()
        .setPayload(std::move(writePayload))
        .setExecute(&ExecuteWriteTask<TFn>)
        .build();
}

template <class TFn> IOTask CreateReadTask(ReadPayload<TFn> readPayload)
{
    return IOTask::Builder<ReadPayload<TFn>>()
        .setPayload(std::move(readPayload))
        .setExecute(&ExecuteReadTask<TFn>)
        .build();
}

} // namespace star::job::tasks::io