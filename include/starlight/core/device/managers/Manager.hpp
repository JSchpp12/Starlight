#pragma once

#include "ManagedHandleContainer.hpp"
#include "ManagerBase.hpp"
#include "device/StarDevice.hpp"
#include "job/TaskManager.hpp"

#include <star_common/Handle.hpp>

#include <array>
#include <cassert>

#include <stack>

namespace star::core::device::manager
{

template <typename TRecord, typename TResourceRequest, size_t TMaxRecordCount>
class Manager : public ManagerBase<TRecord, TResourceRequest>
{
  public:
    explicit Manager(std::string_view resourceHandleName)
        : ManagerBase<TRecord, TResourceRequest>(resourceHandleName), m_records(resourceHandleName) {};

    virtual ~Manager() = default;
    Manager(const Manager &) = delete;
    Manager &operator=(const Manager &) = delete;
    Manager(Manager &&other) noexcept
        : ManagerBase<TRecord, TResourceRequest>(std::move(other)), m_records(std::move(other.m_records)) {};
    Manager &operator=(Manager &&other) noexcept
    {
        if (this != &other)
        {
            ManagerBase<TRecord, TResourceRequest>::operator=(std::move(other));

            m_records = std::move(other.m_records);
        }

        return *this;
    }

    virtual Handle submit(TResourceRequest resource) override
    {
        assert(this->m_device != nullptr && "Init must be called first");

        return insert(std::move(resource));
    }

    TRecord *get(const Handle &handle)
    {
        return &m_records.get(handle);
    }

    const TRecord *get(const Handle &handle) const
    {
        return &m_records.get(handle);
    }

    bool isReady(const Handle &handle) const
    {
        auto *rec = get(handle);
        assert(rec != nullptr);

        return rec->isReady();
    }

    auto &getRecords()
    {
        return m_records;
    }

    virtual void cleanupRender()
    {
        if (this->m_device != nullptr)
        {
            m_records.cleanupAll(this->m_device);
        }
    }

    void deleteRequest(device::StarDevice &device, const Handle &requestHandle)
    {
        assert(requestHandle.getType() == this->getHandleType());
        m_records.cleanup(requestHandle, &device);
        m_records.removeRecord(requestHandle);
    }

  protected:
    star::core::ManagedHandleContainer<TRecord, TMaxRecordCount> m_records;

    Handle insert(TResourceRequest request)
    {
        TRecord record = createRecord(std::move(request));
        return m_records.insert(std::move(record));
    }

    virtual TRecord createRecord(TResourceRequest &&request) const = 0;
};
} // namespace star::core::device::manager