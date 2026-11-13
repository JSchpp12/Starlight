#pragma once

#include "job/tasks/Task.hpp"

#include "starlight/core/logging/LoggingFactory.hpp"
#include "starlight/util/io/PathResolver.hpp"

#include <filesystem>
#include <functional>
#include <string_view>
#include <utility>

namespace star::job::tasks::io
{
inline static constexpr std::string_view IOTaskName = "star::job::tasks::io";

static std::filesystem::path ResolveForIo(const star::io::PathResolver &pathResolver,
                                          const std::filesystem::path &candidate, star::io::PathKind kind,
                                          std::string_view operation) noexcept
{
    auto resolved = pathResolver.resolve(candidate, kind);
    if (!resolved)
    {
        star::core::logging::error("Failed to resolve ", operation, " path: ", resolved.error().toString());
        return candidate;
    }

    return std::move(resolved).value();
}

template <class TFn> struct WritePayload
{
    std::filesystem::path filePath;
    TFn writeFunction;
    /// The worker resolves the target itself. Defaults to the same media
    /// search policy the default IOService uses; callers may override it.
    star::io::PathResolver pathResolver{};

    int operator()()
    {
        std::filesystem::path target = filePath;
        if (!filePath.parent_path().empty())
        {
            auto parent = pathResolver.resolve(filePath.parent_path(), star::io::PathKind::directory);
            if (parent)
            {
                target = *parent / filePath.filename();
            }
            else
            {
                star::core::logging::error("Failed to resolve write directory for '", filePath.string(),
                                           "': ", parent.error().toString());
            }
        }

        star::core::logging::info("Starting file write: " + target.string());
        auto result = std::invoke(writeFunction, target.string());
        star::core::logging::info("Done file write: " + target.string());

        return result;
    }
};

template <class TFn> struct ReadPayload
{
    std::filesystem::path filePath;
    TFn readFunction;
    star::io::PathResolver pathResolver{};
    star::io::PathKind kind{star::io::PathKind::file};

    int operator()()
    {
        const std::filesystem::path resolved = ResolveForIo(pathResolver, filePath, kind, "read");

        star::core::logging::info("Starting file read: " + resolved.string());
        auto result = std::invoke(readFunction, resolved.string());
        star::core::logging::info("Done reading file: " + resolved.string());

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
