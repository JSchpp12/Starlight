#pragma once

#include "Handle.hpp"
#include "job/TaskManager.hpp"
#include "device/system/EventBus.hpp"

#include <array>
#include <cassert>
#include <concepts>
#include <stack>

namespace star::core::device::manager
{
template <typename T, typename TResourceRequest>
concept RecordStructWithHasReady = requires(const T record) {
    { record.isReady() } -> std::same_as<bool>;
} && std::constructible_from<T, TResourceRequest>;

template <typename TRecord, typename TResourceRequest, size_t TMaxRecordCount>
    requires RecordStructWithHasReady<TRecord, TResourceRequest>
class Manager
{
  public:
    Manager() = default; 
    virtual ~Manager() = default;
    Manager(const Manager &) = delete;
    Manager(Manager &&other)
        : m_skippedSpaces(std::move(other.m_skippedSpaces)), m_records(std::move(other.m_records)),
          m_nextSpace(std::move(other.m_nextSpace))
    {
    }
    Manager &operator=(const Manager &) = delete;
    Manager &operator=(Manager &&other)
    {
        if (this != &other)
        {
            m_nextSpace = std::move(other.m_nextSpace);
            m_records = std::move(other.m_records);
            m_skippedSpaces = std::move(other.m_skippedSpaces);
        }
        return *this;
    }
    
    Handle submit(job::TaskManager &taskSystem, system::EventBus &eventBus, TResourceRequest resource)
    {
        TRecord *record = nullptr;
        Handle newHandle;
        insert(std::move(resource), newHandle, record);

        submitTask(newHandle, taskSystem, eventBus, record);

        return newHandle;
    }

    TRecord *get(const Handle &handle)
    {
        assert(handle.getID() < m_records.size() && "Handle is outside of the available storage");
        assert(handle.getType() == getHandleType() && "Invalid handle type provided");

        return &m_records[handle.getID()];
    }

    bool isReady(const Handle &handle)
    {
        auto *rec = get(handle);
        assert(rec != nullptr);

        return rec->isReady();
    }

  protected:
    virtual Handle_Type getHandleType() const = 0;

    std::stack<uint32_t> m_skippedSpaces = std::stack<uint32_t>();
    std::array<TRecord, TMaxRecordCount> m_records = std::array<TRecord, TMaxRecordCount>();
    uint32_t m_nextSpace = 0;

    void insert(TResourceRequest request, Handle &handle, TRecord *&record)
    {
        uint32_t nextSpace = getNextSpace();
        Handle newHandle = Handle(getHandleType(), nextSpace);

        m_records[nextSpace] = TRecord(std::move(request));

        handle = newHandle;
        record = &m_records[nextSpace];
    }

    uint32_t getNextSpace()
    {
        uint32_t nextSpace = 0;

        if (m_nextSpace > m_records.size())
        {
            if (m_skippedSpaces.empty())
            {
                throw std::runtime_error("Storage is full");
            }
            else
            {
                nextSpace = m_skippedSpaces.top();
                m_skippedSpaces.pop();
            }
        }
        else
        {
            nextSpace = m_nextSpace;
            m_nextSpace++;
        }

        return nextSpace;
    }

    virtual void submitTask(const Handle &handle, job::TaskManager &taskSystem, system::EventBus &eventBus, TRecord *storedRecord) = 0;
};
} // namespace star::core::device::manager