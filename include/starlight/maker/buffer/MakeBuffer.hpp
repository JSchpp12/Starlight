#pragma once

#include "starlight/wrappers/graphics/StarBuffers/Buffer.hpp"

#include <star_common/Maker.hpp>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <utility>

namespace star::maker::buffer
{
/// @brief Error produced when a buffer cannot be created.
struct BufferMakeError
{
    enum class Cause
    {
        invalid_config,
        out_of_memory,
        device_lost
    };

    Cause cause;
    std::string detail;
    /// @brief Number of buffers make() constructed at dst before the failure. Slots [0, built) hold live buffers; the
    /// remaining slots hold whatever was there before the call.
    size_t built = 0;
};

/// @brief Builds a buffer from a newly allocated Vulkan buffer.
struct MakeBuffer : public star::common::Maker<star::StarBuffers::Buffer, BufferMakeError>
{
    explicit MakeBuffer(core::device::StarDevice &device);

    MakeBuffer &setCreateInfo(VmaAllocationCreateInfo nAllocInfo, vk::BufferCreateInfo nCreateInfo,
                              std::string nAllocationName);

    MakeBuffer &setInstanceCount(uint32_t nInstanceCount);

    MakeBuffer &setInstanceSize(vk::DeviceSize nInstanceSize);

    MakeBuffer &setMinOffsetAlignment(vk::DeviceSize nMinOffsetAlignment);

    /// @brief Build num buffers in place at dst.
    /// @return The number of buffers built, or an error whose `built` member reports how many
    ///         buffers were constructed at dst before the failure.
    [[nodiscard]] std::expected<size_t, BufferMakeError> make(star::StarBuffers::Buffer *dst,
                                                              size_t num) noexcept override;

  private:
    star::StarBuffers::Buffer createBuffer();

    core::device::StarDevice &m_device;
    std::optional<star::StarBuffers::Buffer::Creators> m_createNewAllocationInfo = std::nullopt;
    uint32_t m_instanceCount = 0;
    vk::DeviceSize m_instanceSize = 0;
    vk::DeviceSize m_minOffsetAlignment = 1;
};
} // namespace star::maker::buffer
