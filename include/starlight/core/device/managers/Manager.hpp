#pragma once

#include "ManagedHandleContainer.hpp"
#include "ManagerBase.hpp"
#include "device/StarDevice.hpp"
#include "job/TaskManager.hpp"
#include <star_common/EventBus.hpp>
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
    Manager(std::string_view resourceHandleName)
        : ManagerBase<TRecord, TResourceRequest>(resourceHandleName), m_records(resourceHandleName){};

    virtual ~Manager() = default;
    Manager(const Manager &) = delete;
    Manager &operator=(const Manager &) = delete;
    Manager(Manager &&other) noexcept
        : ManagerBase<TRecord, TResourceRequest>(std::move(other)), m_records(std::move(other.m_records)),
          m_deviceEventBus(other.m_deviceEventBus){};
    Manager &operator=(Manager &&other) noexcept
    {
        if (this != &other)
        {
            ManagerBase<TRecord, TResourceRequest>::operator=(std::move(other));

            m_records = std::move(other.m_records);
            m_deviceEventBus = other.m_deviceEventBus;
        }

        return *this;
    }

    virtual void init(device::StarDevice *device, common::EventBus &eventBus)
    {
        ManagerBase<TRecord, TResourceRequest>::init(device);

        m_deviceEventBus = &eventBus;
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

    bool isReady(const Handle &handle)
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
    common::EventBus *m_deviceEventBus = nullptr;

    Handle insert(TResourceRequest request)
    {
        TRecord record = createRecord(std::move(request));
        return m_records.insert(std::move(record));
    }

    virtual TRecord createRecord(TResourceRequest &&request) const = 0;

  private:
    using ManagerBase<TRecord, TResourceRequest>::init;
};
} // namespace star::core::device::manager