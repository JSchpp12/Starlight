#pragma once

#include "core/graphics/GPUWorkSyncInfo.hpp"
#include "starlight/job/tasks/TransferTask.hpp"

#include <star_common/Handle.hpp>
#include <star_common/IServiceCommandWithReply.hpp>

namespace star::command::transfer
{
namespace submit_transfer
{
inline constexpr const char *GetUniqueTypeName()
{
    return "ScSt";
}
} // namespace submit_transfer

struct SubmitTransferResult
{
    /// Signal raised when this submission's GPU work retires.
    star::core::graphics::SemaphoreSyncPoint semaphore;
    /// Semaphore-manager record of the timeline `semaphore` belongs to.
    star::Handle semaphoreRecord;
    uint32_t queueFamilyIndex{0};
    uint32_t workerId{0};
};

struct SubmitTransferTask : public star::common::IServiceCommandWithReply<SubmitTransferResult>
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return submit_transfer::GetUniqueTypeName();
    }

    explicit SubmitTransferTask(job::tasks::transfer::TransferTask task) : task(std::move(task))
    {
    }

    job::tasks::transfer::TransferTask task;
};
} // namespace star::command::transfer