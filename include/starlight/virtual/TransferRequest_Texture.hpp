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

    /// @brief Called by the worker once the transfer submission has been issued and the texture published.
    virtual void onTransferComplete()
    {
    }
};
} // namespace star::TransferRequest