#pragma once

#include "StarTextures/Texture.hpp"
#include "TransferRequest_Memory.hpp"
#include "core/exceptions/ResourceCreationError.hpp"

#include <vulkan/vulkan.hpp>

#include <cassert>
#include <expected>
#include <future>
#include <optional>
#include <string>
#include <utility>

namespace star::TransferRequest
{
class Texture : private Memory<star::StarTextures::Texture>
{
  public:
    Texture() = default;
    virtual ~Texture() = default;

    using Memory<star::StarTextures::Texture>::onTransferFailed;

    virtual void prep() override {};

    /// Human-readable identity of the texture being transferred. Used by the transfer workers for debug logging so each
    /// record can be traced back to a specific texture. Requests without a source file can use the default.
    virtual std::string getDebugDescription() const
    {
        return "unnamed texture";
    }

    virtual std::unique_ptr<StarBuffers::Buffer> createStagingBuffer(
        core::device::StarDevice &device) const override = 0;

    virtual std::unique_ptr<star::StarTextures::Texture> createFinal(
        core::device::StarDevice &device, const std::vector<uint32_t> &transferQueueFamilyIndex) const override = 0;

    virtual void copyFromTransferSRCToDST(StarBuffers::Buffer &srcBuffer, star::StarTextures::Texture &dst,
                                          vk::CommandBuffer &commandBuffer) const override = 0;

    virtual void writeDataToStageBuffer(star::StarBuffers::Buffer &stagingBuffer) const override = 0;

    /// @brief True when this request needs the staging buffer to outlive the transfer submission.
    /// @details The worker normally frees staging as soon as the transfer command buffer's fence retires. A request
    /// whose real GPU work runs later (for example a conversion dispatch recorded into a render phase) sets this from
    /// its constructor, before submission, so the TransferService registers a buffer-manager slot for the staging
    /// buffer. The worker then publishes the created buffer into that slot instead of freeing it at the fence.
    bool takesStagingOwnership = false;

    /// @brief Slot the TransferService registered with the buffer manager, filled by the worker after submission.
    /// @details Holds the staging buffer, or the exception that explains why it could not be produced.
    std::optional<std::promise<std::expected<StarBuffers::Buffer, core::ResourceCreationError>>> stagingBufferSlot{};

    /// @brief Publish the staging buffer into the manager slot after submission.
    /// @details Only called when takesStagingOwnership is true. The worker passes its buffer by reference and moves it
    /// into the slot; the manager keeps it alive and its handle was handed to the caller, which releases it once the
    /// recorded work on it has completed.
    virtual void adoptStagingBuffer(StarBuffers::Buffer &stagingBuffer)
    {
        assert(stagingBufferSlot.has_value() &&
               "A request taking staging ownership must have a buffer-manager slot registered by the TransferService");
        stagingBufferSlot->set_value(
            std::expected<StarBuffers::Buffer, core::ResourceCreationError>{std::move(stagingBuffer)});
        stagingBufferSlot.reset();
    }
};
} // namespace star::TransferRequest