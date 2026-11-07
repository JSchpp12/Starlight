#pragma once

#include "starlight/job/tasks/TransferTask.hpp"
#include "starlight/wrappers/graphics/StarSemaphore.hpp"

#include <star_common/Handle.hpp>
#include <star_common/IServiceCommandWithReply.hpp>

#include <optional>

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
    star::StarSemaphore semaphore;
    uint32_t queueFamilyIndex{0};
    uint32_t workerId{0};
    /// Buffer-manager slot for the staging buffer, present when the request takes staging ownership.
    std::optional<star::Handle> stagingBuffer{};
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