#pragma once

#include "Handle.hpp"
#include "ManagedHandleContainer.hpp"
#include "ManagerBase.hpp"
#include "device/StarDevice.hpp"
#include "device/system/EventBus.hpp"
#include "job/TaskManager.hpp"

#include <array>
#include <cassert>

#include <stack>

namespace star::core::device::manager
{

template <typename TRecord, typename TResourceRequest, star::Handle_Type THandleType, size_t TMaxRecordCount>
class Manager : public ManagerBase<TRecord, TResourceRequest, THandleType>
{
  public:
    Manager() = default;
    virtual ~Manager() = default;
    Manager(const Manager &) = delete;
    Manager &operator=(const Manager &) = delete;
    Manager(Manager &&) = default;
    Manager &operator=(Manager &&) = default;

    virtual Handle submit(device::StarDevice &device, TResourceRequest resource) override
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

    LinearHandleContainer<TRecord, THandleType, TMaxRecordCount> &getRecords()
    {
        return m_records;
    }

    void cleanupRender(device::StarDevice &device)
    {
        m_records.cleanupAll(&device);
    }

    void deleteRequest(device::StarDevice &device, const Handle &requestHandle)
    {
        assert(requestHandle.getType() == THandleType);
        m_records.cleanup(requestHandle, &device); 
        m_records.removeRecord(requestHandle); 
    }

  protected:
    star::core::ManagedHandleContainer<TRecord, THandleType, TMaxRecordCount> m_records;

    Handle insert(device::StarDevice &device, TResourceRequest request)
    {
        TRecord record = createRecord(device, std::move(request));
        return m_records.insert(std::move(record));
    }

    virtual TRecord createRecord(device::StarDevice &device, TResourceRequest &&request) const = 0;
};
} // namespace star::core::device::manager