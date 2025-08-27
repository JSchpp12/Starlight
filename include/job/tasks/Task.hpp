#pragma once

#include "TaskTracker.hpp"
#include "complete_tasks/CompleteTask.hpp"

#include "boost/atomic/atomic.hpp"

#include <stdexcept>

namespace star::job::tasks
{
using ExecuteFunction = void (*)(void *);
using DestructorFunction = void (*)(void *);
using MovePayloadFunction = void (*)(void *, void *);
using CreateCompleteTaskFunction = star::job::complete_tasks::CompleteTask<>* (*)(void *);

template <size_t StorageBytes = 128 - sizeof(ExecuteFunction) - sizeof(DestructorFunction) -
                                sizeof(MovePayloadFunction) - sizeof(CreateCompleteTaskFunction) - sizeof(uint16_t) - 8,
          size_t StorageAlign = alignof(std::max_align_t)>
class Task
{
  public:
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
        Builder &setCreateCompleteTaskFunction(CreateCompleteTaskFunction completeTaskFunction)
        {
            m_createCompleteTaskFunction = completeTaskFunction;
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
            task.m_createCompleteFunction = m_createCompleteTaskFunction;

            return task;
        }

      private:
        ExecuteFunction m_executeFunction = nullptr;
        DestructorFunction m_destroyPayloadFunction = nullptr;
        MovePayloadFunction m_movePayloadFunction = nullptr;
        CreateCompleteTaskFunction m_createCompleteTaskFunction = nullptr;

        bool m_hasData = false;
        PayloadType m_data;
    };

    Task() = default;
    Task(Task &&other) noexcept
    {
        if (other.m_movePayloadFunction)
            other.m_movePayloadFunction(m_data, other.m_data);

        m_executeFunction = other.m_executeFunction;
        m_movePayloadFunction = other.m_movePayloadFunction;
        m_destroyPayloadFunction = other.m_destroyPayloadFunction;
        m_createCompleteFunction = other.m_createCompleteFunction;

        other.m_executeFunction = nullptr;
        other.m_destroyPayloadFunction = nullptr;
        other.m_movePayloadFunction = nullptr;
        other.m_createCompleteFunction = nullptr;
    }
    Task &operator=(Task &&other) noexcept 
    {
        if (this != &other)
        {
            if (m_destroyPayloadFunction)
                m_destroyPayloadFunction(m_data);

            m_executeFunction = other.m_executeFunction;
            m_destroyPayloadFunction = other.m_destroyPayloadFunction;
            m_movePayloadFunction = other.m_movePayloadFunction;
            m_createCompleteFunction = other.m_createCompleteFunction;

            if (m_movePayloadFunction)
                m_movePayloadFunction(m_data, other.m_data);

            other.m_executeFunction = nullptr;
            other.m_destroyPayloadFunction = nullptr;
            other.m_movePayloadFunction = nullptr;
            other.m_createCompleteFunction = nullptr;
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

    uint16_t id = 0;

    void run()
    {
        m_executeFunction(payload());
    }

    complete_tasks::CompleteTask<> *getCompleteMessage(){
        if (m_createCompleteFunction)
            return m_createCompleteFunction(payload()); 

        return nullptr; 
    }

  private:
    ExecuteFunction m_executeFunction = nullptr;
    DestructorFunction m_destroyPayloadFunction = nullptr;
    MovePayloadFunction m_movePayloadFunction = nullptr;
    CreateCompleteTaskFunction m_createCompleteFunction = nullptr;
    alignas(StorageAlign) std::byte m_data[StorageBytes];

    void *payload() noexcept
    {
        return static_cast<void *>(m_data);
    }
    const void *payload() const noexcept
    {
        return static_cast<const void *>(m_data);
    }
};
} // namespace star::job::tasks