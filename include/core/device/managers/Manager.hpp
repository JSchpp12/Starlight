#pragma once

#include "Handle.hpp"
#include "device/StarDevice.hpp"
#include "device/system/EventBus.hpp"
#include "job/TaskManager.hpp"

#include <array>
#include <cassert>
#include <concepts>
#include <stack>

namespace star::core::device::manager
{
template <typename T, typename TResourceRequest>
concept RecordStructWithHasReady = requires(const T record, device::StarDevice &device) {
    { record.isReady() } -> std::same_as<bool>;
};
template <typename T, typename TResourceRequest>
concept RecordStructHasCleanup = requires(T record, device::StarDevice &device) {
    { record.cleanupRender(device) } -> std::same_as<void>;
};

template <typename TRecord, typename TResourceRequest, size_t TMaxRecordCount>
    requires RecordStructWithHasReady<TRecord, TResourceRequest> &&
             RecordStructHasCleanup<TRecord, TResourceRequest>
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

    Handle submit(device::StarDevice &device, job::TaskManager &taskSystem, system::EventBus &eventBus,
                  TResourceRequest resource)
    {
        TRecord *record = nullptr;
        Handle newHandle;
        insert(device, newHandle, std::move(resource), record);

        submitTask(device, newHandle, taskSystem, eventBus, record);

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

    std::array<TRecord, TMaxRecordCount> &getRecords()
    {
        return m_records;
    }

    void cleanupRender(device::StarDevice &device)
    {
        for (auto &record : m_records)
        {
            record.cleanupRender(device);
        }
    }

  protected:
    virtual Handle_Type getHandleType() const = 0;

    std::stack<uint32_t> m_skippedSpaces = std::stack<uint32_t>();
    std::array<TRecord, TMaxRecordCount> m_records = std::array<TRecord, TMaxRecordCount>();
    uint32_t m_nextSpace = 0;

    void insert(device::StarDevice &device, Handle &handle, TResourceRequest request, TRecord *&record)
    {
        uint32_t nextSpace = getNextSpace();
        Handle newHandle = Handle{.type = getHandleType(), .id = nextSpace};

        m_records[nextSpace] = createRecord(device, std::move(request));

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

    virtual TRecord createRecord(device::StarDevice &device, TResourceRequest &&request) const = 0;

    virtual void submitTask(device::StarDevice &device, const Handle &handle, job::TaskManager &taskSystem,
                            system::EventBus &eventBus, TRecord *storedRecord) {};
};
} // namespace star::core::device::manager