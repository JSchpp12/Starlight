#pragma once

#include "core/graphics/GPUWorkSyncInfo.hpp"
#include "starlight/virtual/TransferRequest_Texture.hpp"

#include <star_common/Handle.hpp>
#include <star_common/IServiceCommandWithReply.hpp>

#include <memory>
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
    /// Signal raised when the transfer's GPU work retires.
    star::core::graphics::SemaphoreSyncPoint semaphore;
    /// Semaphore-manager record of the timeline `semaphore` belongs to.
    star::Handle semaphoreRecord;
};

/// @brief Submit a texture transfer whose created texture is published into the Image manager.
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
