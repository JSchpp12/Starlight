#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>

namespace star::job::complete_tasks
{
using EngineOnCompleteFuntion = void (*)(void *);
using DestroyPayloadFunction = void (*)(void *);
using MovePayloadFunction = void (*)(void *, void *);

template <size_t StorageBytes =
              128 - sizeof(EngineOnCompleteFuntion) - sizeof(DestroyPayloadFunction) - sizeof(MovePayloadFunction),
          size_t StorageAlign = alignof(std::max_align_t)>
class CompleteTask
{
  public:
    template <typename PayloadType> class Builder
    {
      public:
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
        Builder &setCompleteFunction(EngineOnCompleteFuntion onCompleteFunction)
        {
            m_onCompleteFunction = onCompleteFunction;
            return *this;
        }
        Builder &setDestroyFunction(DestroyPayloadFunction destroyPayloadFunction)
        {
            m_destroyPayloadFunction = destroyPayloadFunction;
            return *this;
        }
        Builder &setMovePayloadFunction(MovePayloadFunction movePayloadFunction)
        {
            m_movePayloadFunction = movePayloadFunction;
            return *this;
        }
        CompleteTask<> build()
        {
            static_assert(sizeof(PayloadType) <= StorageBytes,
                          "Payload too large for message inline storage restrictions");
            static_assert(alignof(PayloadType) <= StorageAlign,
                          "Payload alignment too strict against restricted alignment");

            if (!m_hasData)
                throw std::runtime_error("Data must be provided");
            if (!m_onCompleteFunction)
                throw std::runtime_error("On complete function must be provided");
            if (!m_destroyPayloadFunction)
                m_destroyPayloadFunction = &Builder<PayloadType>::DefaultDestroyPayload;
            if (!m_movePayloadFunction)
                m_movePayloadFunction = &Builder<PayloadType>::DefaultMovePayload;

            CompleteTask<> complete;
            new (complete.m_data) PayloadType(std::forward<PayloadType>(m_data));
            complete.m_onCompleteFunction = m_onCompleteFunction;
            complete.m_movePayloadFunction = m_movePayloadFunction;
            complete.m_destroyPayloadFunction = m_destroyPayloadFunction;

            return complete;
        }

      private:
        EngineOnCompleteFuntion m_onCompleteFunction = nullptr;
        MovePayloadFunction m_movePayloadFunction = nullptr;
        DestroyPayloadFunction m_destroyPayloadFunction = nullptr;

        bool m_hasData = false;
        PayloadType m_data;
    };

    CompleteTask() = default;
    CompleteTask(CompleteTask &&other) noexcept
    {
        if (other.m_movePayloadFunction)
            other.m_movePayloadFunction(m_data, other.m_data);

        m_onCompleteFunction = other.m_onCompleteFunction;
        m_movePayloadFunction = other.m_movePayloadFunction;
        m_destroyPayloadFunction = other.m_destroyPayloadFunction;

        other.m_onCompleteFunction = nullptr;
        other.m_movePayloadFunction = nullptr;
        other.m_destroyPayloadFunction = nullptr;
    }
    CompleteTask &operator=(CompleteTask &&other) noexcept
    {
        if (this != other)
        {
            if (m_destroyPayloadFunction)
                m_destroyPayloadFunction(m_data);

            m_onCompleteFunction = other.m_onCompleteFunction;
            m_destroyPayloadFunction = other.m_destroyPayloadFunction;
            m_movePayloadFunction = other.m_movePayloadFunction;

            if (m_movePayloadFunction)
                m_movePayloadFunction(m_data, other.m_data);

            other.m_onCompleteFunction = nullptr;
            other.m_destroyPayloadFunction = nullptr;
            other.m_movePayloadFunction = nullptr;
        }

        return *this;
    }
    CompleteTask &operator=(const CompleteTask &) = delete;
    CompleteTask(const CompleteTask &) = delete;
    ~CompleteTask()
    {
        if (m_destroyPayloadFunction)
            m_destroyPayloadFunction(m_data);
    }

    void run()
    {
        m_onCompleteFunction(payload());
    }

  private:
    EngineOnCompleteFuntion m_onCompleteFunction = nullptr;
    MovePayloadFunction m_movePayloadFunction = nullptr;
    DestroyPayloadFunction m_destroyPayloadFunction = nullptr;
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
} // namespace star::job::complete_tasks