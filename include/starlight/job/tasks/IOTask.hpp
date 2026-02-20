#pragma once

#include "job/tasks/Task.hpp"

#include <functional>
#include <future>
#include <string_view>

namespace star::job::tasks::io
{
inline static constexpr std::string_view IOTaskName = "star::job::tasks::io";

struct WritePayload
{
    std::string filePath;
    std::function<void(const std::string &)> writeFileFunction;
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
        return std::invoke(readFunction, filePath);
    }
};

using IOTask = star::job::tasks::Task<128, alignof(std::max_align_t)>;

void ExecuteWriteTask(void *p);

std::optional<star::job::complete_tasks::CompleteTask> CreateWriteTaskComplete(void *p);

IOTask CreateIOTask(std::string filePath, std::function<void(const std::string &)> writeFileFunction);

template <class TFn> void ExecuteReadTask(void *p)
{
    auto *payload = static_cast<ReadPayload<TFn> *>(p);
    payload->operator()();
}

template <class TFn> IOTask CreateReadTask(ReadPayload<TFn> readPayload)
{
    return IOTask::Builder<ReadPayload<TFn>>()
        .setPayload(std::move(readPayload))
        .setExecute(&ExecuteReadTask<TFn>)
        .build();
}

} // namespace star::job::tasks::io