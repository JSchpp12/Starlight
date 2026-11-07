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
constexpr std::string_view GetBufferHandleTypeName = "star::core::manager::buffer";
constexpr std::string_view GetBufferEventTypeName = "star::core::manager::event::buffer";

/// @brief A buffer slot whose producer has not published its result yet.
/// @details A child thread owns the promise side and writes the created buffer into it. The manager only stores the
/// future, so it never has to touch the slot from the producer thread. The manager polls this future on access and
/// replaces the slot's variant alternative with the resolved buffer once it is ready.
class Pending
{
  public:
    using Buffer = star::StarBuffers::Buffer;

    /// @brief Value the producer publishes: the buffer, or a failure message.
    /// @details Encoding failure as a value means a producer that is about to die can publish the reason without the
    /// consumer having to distinguish "failed" from "not published yet" by throwing.
    using Result = std::expected<Buffer, std::string>;

    explicit Pending(std::future<Result> future) : m_future(std::move(future))
    {
    }

    Pending(Pending &&) noexcept = default;
    Pending &operator=(Pending &&) noexcept = default;
    Pending(const Pending &) = delete;
    Pending &operator=(const Pending &) = delete;

    /// @brief Non-blocking check for a published result.
    bool isReady() const noexcept
    {
        // wait_for() is undefined on an invalid future (e.g. one already consumed or created from a broken promise).
        return m_future.valid() && m_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    bool isValid() const noexcept
    {
        return m_future.valid();
    }

    /// @brief Consume the published result. Never throws: a producer destroyed without publishing is reported as an
    /// unexpected so the caller can fail soft instead of waiting forever.
    Result take() noexcept
    {
        try
        {
            return m_future.get();
        }
        catch (const std::exception &e)
        {
            return std::unexpected(std::string{"buffer producer failed: "} + e.what());
        }
    }

  private:
    std::future<Result> m_future;
};

using BufferSlot = std::variant<star::StarBuffers::Buffer, Pending>;

struct BufferRequest
{
    BufferSlot buffer;

    explicit BufferRequest(star::StarBuffers::Buffer buffer) : buffer(std::move(buffer))
    {
    }
    explicit BufferRequest(Pending pending) : buffer(std::move(pending))
    {
    }
    explicit BufferRequest(std::future<Pending::Result> future) : buffer(Pending{std::move(future)})
    {
    }
    explicit BufferRequest(wrappers::graphics::policies::GenericBufferCreateAllocatePolicy createPolicy)
        : buffer(createPolicy())
    {
    }
};

struct BufferRecord
{
    /// @brief Lazily populated slot. Mutable because resolving a ready future is a cache fill, not a state change
    /// visible to callers, so it may happen through a const manager reference.
    mutable BufferSlot buffer;

    /// @brief True once a concrete buffer is present, including the blank buffer used when a producer failed.
    bool isReady() const noexcept
    {
        if (std::holds_alternative<star::StarBuffers::Buffer>(buffer))
            return true;

        const auto *pending = std::get_if<Pending>(&buffer);
        return pending != nullptr && pending->isReady();
    }

    /// @brief The resolved buffer, or nullptr while the slot is still pending.
    /// @details A failed producer leaves a default-constructed Buffer here: consumers must check
    /// `getVulkanBuffer()` / `getBufferSize()` rather than trusting the pointer.
    star::StarBuffers::Buffer *getBuffer()
    {
        return std::get_if<star::StarBuffers::Buffer>(&buffer);
    }
    const star::StarBuffers::Buffer *getBuffer() const
    {
        return std::get_if<star::StarBuffers::Buffer>(&buffer);
    }

    void cleanupRender(device::StarDevice &device)
    {
        if (auto *pending = std::get_if<Pending>(&buffer))
        {
            // A slot still unpublishing at teardown owns nothing from the manager's perspective. Its producer must
            // publish (and clean up) before the device goes away; blocking here would hide that ownership mistake.
            if (!pending->isReady())
            {
                return;
            }

            auto result = pending->take();
            if (result.has_value())
                buffer = std::move(*result);
            else
                buffer = star::StarBuffers::Buffer{};
        }

        if (auto *resolved = std::get_if<star::StarBuffers::Buffer>(&buffer))
            resolved->cleanupRender(device.getVulkanDevice());
    }
};

class Buffer : public ManagerEventBusTies<BufferRecord, BufferRequest, 30>
{
  public:
    using Base = ManagerEventBusTies<BufferRecord, BufferRequest, 30>;

    Buffer();
    Buffer(const Buffer &&) = delete;
    Buffer &operator=(const Buffer &&) = delete;
    Buffer(Buffer &&other) = default;
    Buffer &operator=(Buffer &&other) = default;

    virtual ~Buffer() = default;

    using Base::get;

    /// @brief Access a slot, resolving a pending future into a concrete buffer when one is ready.
    BufferRecord *get(const Handle &handle) override;
    const BufferRecord *get(const Handle &handle) const override;

  private:
    virtual BufferRecord createRecord(BufferRequest &&request) const override;

    static void resolve(const BufferRecord &record) noexcept;
};
} // namespace star::core::device::manager
