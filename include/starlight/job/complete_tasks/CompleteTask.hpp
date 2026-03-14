#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>

namespace star::job::complete_tasks
{

using DestroyPayloadFunction = void (*)(void *);
using MovePayloadFunction = void (*)(void *, void *);
using EngineOnCompleteFunction = void (*)(void *, void *, void *, void *, void *);

struct EngineContext
{
    void *device = nullptr;
    void *taskSystem = nullptr;
    void *eventBus = nullptr;
    void *graphicsManagers = nullptr;
};

class CompleteTask
{
  public:
    static constexpr size_t StorageBytes =
        128 - sizeof(EngineOnCompleteFunction) - sizeof(DestroyPayloadFunction) - sizeof(MovePayloadFunction);
    static constexpr size_t StorageAlign = alignof(std::max_align_t);

    template <typename PayloadType> class Builder
    {
      public:
        [[nodiscard]] Builder &setPayload(const PayloadType &data)
        {
            m_data = data;
            return *this;
        }

        [[nodiscard]] Builder &setPayload(PayloadType &&data)
        {
            m_data = std::move(data);
            return *this;
        }

        [[nodiscard]] Builder &setDestroyFunction(DestroyPayloadFunction fn)
        {
            m_destroyFn = fn;
            return *this;
        }

        [[nodiscard]] Builder &setMoveFunction(MovePayloadFunction fn)
        {
            m_moveFn = fn;
            return *this;
        }

        [[nodiscard]] Builder &setExecuteFunction(EngineOnCompleteFunction fn)
        {
            m_onCompleteFn = fn;
            return *this;
        }

        [[nodiscard]] CompleteTask build()
        {
            static_assert(sizeof(PayloadType) <= StorageBytes, "Payload too large for CompleteTask inline storage");
            static_assert(alignof(PayloadType) <= StorageAlign,
                          "Payload alignment too strict for CompleteTask storage");

            if (!m_data)
                throw std::runtime_error("Payload must be provided before building");
            if (!m_onCompleteFn)
                throw std::runtime_error("Execute function must be provided before building");

            if (!m_destroyFn)
                m_destroyFn = [](void *p) { static_cast<PayloadType *>(p)->~PayloadType(); };
            if (!m_moveFn)
                m_moveFn = [](void *dest, void *src) {
                    new (dest) PayloadType(std::move(*static_cast<PayloadType *>(src)));
                    static_cast<PayloadType *>(src)->~PayloadType();
                };

            CompleteTask task{};
            new (task.m_data) PayloadType(std::move(*m_data));
            task.m_onCompleteFn = m_onCompleteFn;
            task.m_moveFn = m_moveFn;
            task.m_destroyFn = m_destroyFn;
            return task;
        }

      private:
        EngineOnCompleteFunction m_onCompleteFn = nullptr;
        MovePayloadFunction m_moveFn = nullptr;
        DestroyPayloadFunction m_destroyFn = nullptr;
        std::optional<PayloadType> m_data;
    };

    CompleteTask() = default;

    CompleteTask(CompleteTask &&other) noexcept
    {
        moveFrom(std::move(other));
    }

    CompleteTask &operator=(CompleteTask &&other) noexcept
    {
        if (this != &other)
        {
            destroyPayload();
            moveFrom(std::move(other));
        }
        return *this;
    }

    CompleteTask(const CompleteTask &) = delete;
    CompleteTask &operator=(const CompleteTask &) = delete;

    ~CompleteTask()
    {
        destroyPayload();
    }

    // Must be called on the main engine thread
    void run(const EngineContext &ctx)
    {
        m_onCompleteFn(ctx.device, ctx.taskSystem, ctx.eventBus, ctx.graphicsManagers, payload());
    }

    void reset()
    {
        destroyPayload();
        m_onCompleteFn = nullptr;
        m_moveFn = nullptr;
        m_destroyFn = nullptr;
    }

  private:
    alignas(StorageAlign) std::byte m_data[StorageBytes]{};
    EngineOnCompleteFunction m_onCompleteFn = nullptr;
    MovePayloadFunction m_moveFn = nullptr;
    DestroyPayloadFunction m_destroyFn = nullptr;

    void *payload() noexcept
    {
        return static_cast<void *>(m_data);
    }
    const void *payload() const noexcept
    {
        return static_cast<const void *>(m_data);
    }

    void destroyPayload() noexcept
    {
        if (m_destroyFn)
            m_destroyFn(m_data);
    }

    void moveFrom(CompleteTask &&other) noexcept
    {
        if (other.m_moveFn)
            other.m_moveFn(m_data, other.m_data);

        m_onCompleteFn = other.m_onCompleteFn;
        m_moveFn = other.m_moveFn;
        m_destroyFn = other.m_destroyFn;

        other.m_onCompleteFn = nullptr;
        other.m_moveFn = nullptr;
        other.m_destroyFn = nullptr;
    }
};

static_assert(sizeof(CompleteTask) <= 128, "CompleteTask exceeds expected size budget");

} // namespace star::job::complete_tasks