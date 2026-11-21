#pragma once

#include "core/graphics/GPUWorkSyncInfo.hpp"
#include "starlight/job/tasks/TransferTask.hpp"
#include "starlight/virtual/TransferRequest_Buffer.hpp"

#include <star_common/Handle.hpp>
#include <star_common/IServiceCommandWithReply.hpp>

#include <memory>
#include <string_view>

namespace star::command::transfer
{
namespace submit_buffer_transfer
{
inline constexpr const char *GetUniqueTypeName()
{
    return "ScStb";
}
} // namespace submit_buffer_transfer

/// @brief Result of a buffer transfer whose created buffer is published into the Buffer manager.
struct SubmitBufferResult
{
    /// Buffer-manager record the transfer worker publishes the buffer into.
    star::Handle buffer;
    /// Signal raised when the transfer's GPU work retires.
    star::core::graphics::SemaphoreSyncPoint semaphore;
    /// Semaphore-manager record of the timeline `semaphore` belongs to.
    star::Handle semaphoreRecord;
};

/// @brief Submit a buffer transfer whose created buffer is published into the Buffer manager.
struct SubmitBufferTransfer : public star::common::IServiceCommandWithReply<SubmitBufferResult>
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return submit_buffer_transfer::GetUniqueTypeName();
    }

    explicit SubmitBufferTransfer(
        std::unique_ptr<star::TransferRequest::Buffer> request,
        job::tasks::transfer::TransferPriority priority = job::tasks::transfer::TransferPriority::Standard)
        : request(std::move(request)), priority(priority)
    {
    }

    std::unique_ptr<star::TransferRequest::Buffer> request;
    job::tasks::transfer::TransferPriority priority = job::tasks::transfer::TransferPriority::Standard;
};
} // namespace star::command::transfer
