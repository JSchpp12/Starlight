#pragma once

#include "boost/atomic/atomic.hpp"

#include <stdexcept>

namespace star::job
{
using ExecuteFunction = void (*)(void *);
using DestructorFunction = void (*)(void *);
using MovePayloadFunction = void (*)(void *, void *);
template <size_t StorageBytes = 128 - sizeof(ExecuteFunction) - sizeof(DestructorFunction) - sizeof(MovePayloadFunction), size_t StorageAlign = alignof(std::max_align_t)> class Task
{
  public:
    enum class Status
    {
        Created,
        Submitted,
        Processing,
        Completed
    };

    template <typename PayloadType> class Builder
    {
      public:
        Builder() = default;

        static void DefaultMovePayload(void *dest, void *src)
        {
            new (dest) PayloadType(std::move(*static_cast<PayloadType *>(src)));
            static_cast<PayloadType *>(src)->~PayloadType();
        }

        static void DefaultDestroyPayload(void *data)
        {
            static_cast<PayloadType *>(data)->~PayloadType();
        }

        Builder &setPayload(const PayloadType &data)
        {
            m_hasData = true;
            m_data = data;
            return *this;
        }
        Builder &setPayload(PayloadType &&data)
        {
            m_hasData = true;
            m_data = std::move(data);
            return *this;
        }
        Builder &setExecute(ExecuteFunction execute)
        {
            m_executeFunction = execute;
            return *this;
        }
        Builder &setDestroy(DestructorFunction destroy)
        {
            m_destroyPayloadFunction = destroy;
            return *this;
        }
        Builder &setMovePayload(MovePayloadFunction payloadFunction)
        {
            m_movePayloadFunction = payloadFunction;
            return *this;
        }
        Task<> build()
        {
            static_assert(sizeof(PayloadType) <= StorageBytes, "Payload too large for job inline storage");
            static_assert(alignof(PayloadType) <= StorageAlign, "Payload alignment too strict");

            if (!m_hasData)
                throw std::runtime_error("Data must be provided");

            if (!m_executeFunction)
                throw std::runtime_error("Execute function must be provided");

            if (!m_destroyPayloadFunction)
                m_destroyPayloadFunction = &Builder<PayloadType>::DefaultDestroyPayload;

            if (!m_movePayloadFunction)
                m_movePayloadFunction = &Builder<PayloadType>::DefaultMovePayload;

            Task<> task;
            new (task.m_data) PayloadType(std::forward<PayloadType>(m_data));
            task.m_executeFunction = m_executeFunction;
            task.m_destroyPayloadFunction = m_destroyPayloadFunction;
            task.m_movePayloadFunction = m_movePayloadFunction;

            return task;
        }

      private:
        ExecuteFunction m_executeFunction = nullptr;
        DestructorFunction m_destroyPayloadFunction = nullptr;
        MovePayloadFunction m_movePayloadFunction = nullptr;
        bool m_hasData = false;
        PayloadType m_data;
    };

    Task() noexcept : m_executeFunction(nullptr), m_destroyPayloadFunction(nullptr), m_movePayloadFunction(nullptr)
    {
    }

    Task(Task &&other) noexcept
    {
        if (other.m_movePayloadFunction)
            other.m_movePayloadFunction(m_data, other.m_data);

        m_executeFunction = other.m_executeFunction;
        m_movePayloadFunction = other.m_movePayloadFunction;
        m_destroyPayloadFunction = other.m_destroyPayloadFunction;

        other.m_executeFunction = nullptr;
        other.m_destroyPayloadFunction = nullptr;
        other.m_movePayloadFunction = nullptr;
    }
    Task &operator=(Task &&other)
    {
        if (this != &other)
        {

            if (m_destroyPayloadFunction)
                m_destroyPayloadFunction(m_data);

            m_executeFunction = other.m_executeFunction;
            m_destroyPayloadFunction = other.m_destroyPayloadFunction;
            m_movePayloadFunction = other.m_movePayloadFunction;

            if (m_movePayloadFunction)
            {
                m_movePayloadFunction(m_data, other.m_data);
            }

            other.m_executeFunction = nullptr;
            other.m_destroyPayloadFunction = nullptr;
            other.m_movePayloadFunction = nullptr;
        }
        return *this;
    }
    Task &operator=(const Task &) = delete;
    Task(const Task &) = delete;
    ~Task()
    {
        if (m_destroyPayloadFunction)
            m_destroyPayloadFunction(m_data);
    }

    void run()
    {
        m_executeFunction(payload());
    }

  private:
    ExecuteFunction m_executeFunction = nullptr;
    DestructorFunction m_destroyPayloadFunction = nullptr;
    MovePayloadFunction m_movePayloadFunction = nullptr;

    void *payload() noexcept
    {
        return static_cast<void *>(m_data);
    }
    const void *payload() const noexcept
    {
        return static_cast<const void *>(m_data);
    }

    alignas(StorageAlign) std::byte m_data[StorageBytes];
};
} // namespace star::job