#pragma once

#include "device/managers/ManagerEventBusTies.hpp"
#include "wrappers/graphics/policies/GenericBufferCreateAllocatePolicy.hpp"

#include <chrono>
#include <expected>
#include <future>
#include <string>
#include <utility>
#include <variant>

namespace star::core::device::manager
{
constexpr std::string_view GetBufferHandleTypeName = "stMHBuf";
constexpr std::string_view GetBufferEventTypeName = "stMHEvt";

/// @brief Compile-time name of the buffer resource type, used for handle registration and failure logging.
struct BufferResourceTypeName
{
    static constexpr std::string_view GetUniqueTypeName()
    {
        return GetBufferHandleTypeName;
    }
};

struct BufferRequest : public ManagerResourceRequest<star::StarBuffers::Buffer>
{
    using Base = ManagerResourceRequest<star::StarBuffers::Buffer>;
    using Base::Base;

    explicit BufferRequest(wrappers::graphics::policies::GenericBufferCreateAllocatePolicy createPolicy)
        : Base(createPolicy())
    {
    }
};

struct BufferRecord
{
    star::StarBuffers::Buffer buffer;

    /// @brief A concrete record is always ready; pending futures are tracked by the manager's slot.
    bool isReady() const noexcept
    {
        return true;
    }

    star::StarBuffers::Buffer *getBuffer()
    {
        return &buffer;
    }
    const star::StarBuffers::Buffer *getBuffer() const
    {
        return &buffer;
    }

    void cleanupRender(device::StarDevice &device)
    {
        buffer.cleanupRender(device.getVulkanDevice());
    }
};

class Buffer : public ManagerEventBusTies<BufferRecord, BufferRequest, 30, BufferResourceTypeName>
{
  public:
    using Base = ManagerEventBusTies<BufferRecord, BufferRequest, 30, BufferResourceTypeName>;

    Buffer();
    Buffer(const Buffer &&) = delete;
    Buffer &operator=(const Buffer &&) = delete;
    Buffer(Buffer &&other) = default;
    Buffer &operator=(Buffer &&other) = default;

    virtual ~Buffer() = default;

    using Base::get;

  private:
    virtual BufferRecord createRecord(BufferRequest &&request) const override;
};
} // namespace star::core::device::manager
