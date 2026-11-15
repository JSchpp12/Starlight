#pragma once

#include "job/TransferWorker.hpp"
#include "job/complete_tasks/CompleteTask.hpp"
#include "job/tasks/Task.hpp"

#include <star_common/HandleTypeRegistry.hpp>

#include <boost/atomic.hpp>
#include <vulkan/vulkan.hpp>

#include <memory>
#include <optional>
#include <string_view>

namespace star::job::tasks::transfer
{
inline static constexpr std::string_view TransferTaskName = "stTP";

/// @brief Registry id of the transfer task pool, resolved once and cached.
inline uint16_t GetTransferTaskTypeTag()
{
    static const uint16_t id = star::common::HandleTypeRegistry::instance().registerType(TransferTaskName);
    return id;
}

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

using TransferTask = star::job::tasks::Task<>;

std::optional<star::job::complete_tasks::CompleteTask> CreateTransferComplete(void *p);

void ExecuteTransferTask(void *p);

inline TransferTask CreateTransferTask(TransferPayload payload)
{
    return TransferTask::Builder<TransferPayload>()
        .setPayload(std::move(payload))
        .setExecute(&ExecuteTransferTask)
        .setCreateCompleteTaskFunction(&CreateTransferComplete)
        .setTypeTag(GetTransferTaskTypeTag())
        .build();
}
} // namespace star::job::tasks::transfer
