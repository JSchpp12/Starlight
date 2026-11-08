#pragma once

#include "starlight/virtual/TransferRequest_Texture.hpp"
#include "starlight/wrappers/graphics/StarSemaphore.hpp"

#include <star_common/Handle.hpp>
#include <star_common/IServiceCommandWithReply.hpp>

#include <memory>
#include <optional>
#include <string_view>

namespace star::command::transfer
{
namespace submit_texture_transfer
{
inline constexpr const char *GetUniqueTypeName()
{
    return "ScStt";
}
} // namespace submit_texture_transfer

/// @brief Result of a texture transfer whose created texture is published into the Image manager.
struct SubmitTextureResult
{
    /// Image-manager record the transfer worker publishes the texture into.
    star::Handle image;
    /// Semaphore the transfer signals when its GPU work retires.
    star::StarSemaphore semaphore;
    /// Buffer-manager slot for the staging buffer, present when the request takes staging ownership.
    std::optional<star::Handle> stagingBuffer{};
};

/// @brief Submit a texture transfer whose created texture is published into the Image manager.
/// @details The TransferService creates the Image manager record (with a pending future) on the submitting thread and
/// hands the request's slot to the transfer worker, which fills it once the texture exists. No ManagerRenderResource
/// record is allocated for the texture; readiness is the Image record's future.
struct SubmitTextureTransfer : public star::common::IServiceCommandWithReply<SubmitTextureResult>
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return submit_texture_transfer::GetUniqueTypeName();
    }

    explicit SubmitTextureTransfer(std::unique_ptr<star::TransferRequest::Texture> request)
        : request(std::move(request))
    {
    }

    std::unique_ptr<star::TransferRequest::Texture> request;
};
} // namespace star::command::transfer
