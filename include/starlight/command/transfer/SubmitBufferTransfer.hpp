#pragma once

#include "starlight/job/tasks/TransferTask.hpp"
#include "starlight/virtual/TransferRequest_Buffer.hpp"
#include "starlight/wrappers/graphics/StarSemaphore.hpp"

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
    /// Semaphore the transfer signals when its GPU work retires.
    star::StarSemaphore semaphore;
};

/// @brief Submit a buffer transfer whose created buffer is published into the Buffer manager.
/// @details The TransferService creates the Buffer manager record (with a pending future) on the submitting thread and
/// hands the request's slot to the transfer worker, which fills it once the buffer exists. No ManagerRenderResource
/// record is allocated for the buffer; readiness is the Buffer record's future.
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
