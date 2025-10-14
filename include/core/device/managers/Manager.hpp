#pragma once

#include "Handle.hpp"
#include "HandleContainer.hpp"
#include "device/StarDevice.hpp"
#include "device/system/EventBus.hpp"
#include "job/TaskManager.hpp"

#include <array>
#include <cassert>
#include <concepts>
#include <stack>

namespace star::core::device::manager
{
template <typename T>
concept RecordStructHasCleanup = requires(T record, device::StarDevice &device) {
    { record.cleanupRender(device) } -> std::same_as<void>;
};

template <typename TRecord, typename TResourceRequest, star::Handle_Type THandleType, size_t TMaxRecordCount>
    requires RecordStructHasCleanup<TRecord>
class Manager
{
  public:
    Manager() = default;
    Manager(Manager &&) noexcept = default;
    Manager &operator=(Manager &&) noexcept = default;
    Manager(const Manager &) = delete;
    Manager &operator=(const Manager &) = delete;

    virtual ~Manager() = default;

    virtual Handle submit(device::StarDevice &device, job::TaskManager &taskSystem, system::EventBus &eventBus,
                          TResourceRequest resource)
    {
        return insert(device, std::move(resource));
    }

    TRecord *get(const Handle &handle)
    {
        return &m_records.get(handle);
    }

    bool isReady(const Handle &handle)
    {
        auto *rec = get(handle);
        assert(rec != nullptr);

        return rec->isReady();
    }

    HandleContainer<TRecord, THandleType, TMaxRecordCount> &getRecords()
    {
        return m_records;
    }

    void cleanupRender(device::StarDevice &device)
    {
        m_records.cleanupAll(&device);
    }

  protected:
    star::core::HandleContainer<TRecord, THandleType, TMaxRecordCount> m_records;

    Handle insert(device::StarDevice &device, TResourceRequest request)
    {
        TRecord record = createRecord(device, std::move(request));
        return m_records.insert(std::move(record));
    }

    virtual TRecord createRecord(device::StarDevice &device, TResourceRequest &&request) const = 0;
};
} // namespace star::core::device::manager