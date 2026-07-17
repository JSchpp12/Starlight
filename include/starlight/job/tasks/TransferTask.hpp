#pragma once

#include "job/TransferWorker.hpp"
#include "job/complete_tasks/CompleteTask.hpp"
#include "job/tasks/Task.hpp"

#include <boost/atomic.hpp>
#include <vulkan/vulkan.hpp>

#include <memory>
#include <optional>
#include <string_view>

namespace star::job::tasks::transfer
{
inline static constexpr std::string_view TransferTaskName = "stTP";

enum class TransferPriority
{
    Standard,
    High
};

struct TransferPayload
{
    TransferPriority priority = TransferPriority::Standard;
    std::unique_ptr<job::TransferManagerThread::InterThreadRequest> request;

    TransferPayload() = default;
    explicit TransferPayload(TransferPriority priority,
                             std::unique_ptr<job::TransferManagerThread::InterThreadRequest> request)
        : priority(priority), request(std::move(request))
    {
    }
    TransferPayload(TransferPayload &&other) = default;
    TransferPayload &operator=(TransferPayload &&other) = default;
    TransferPayload(const TransferPayload &) = delete;
    TransferPayload &operator=(const TransferPayload &) = delete;
    ~TransferPayload() = default;
};

using TransferTask = star::job::tasks::Task<128, alignof(std::max_align_t)>;

std::optional<star::job::complete_tasks::CompleteTask> CreateTransferComplete(void *p);

void ExecuteTransferTask(void *p);

inline TransferTask CreateTransferTask(TransferPayload payload)
{
    static_assert(sizeof(TransferPayload) <= 128, "TransferPayload exceeds inline task storage");

    return TransferTask::Builder<TransferPayload>()
        .setPayload(std::move(payload))
        .setExecute(&ExecuteTransferTask)
        .setCreateCompleteTaskFunction(&CreateTransferComplete)
        .build();
}
} // namespace star::job::tasks::transfer
