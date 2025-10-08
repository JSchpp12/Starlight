#pragma once

#include "Manager.hpp"

namespace star::core::device::manager
{
template <typename T>
concept RecordStructWithHasReady = requires(const T record) {
    { record.isReady() } -> std::same_as<bool>;
};
template <typename TRecord, typename TResourceRequest, size_t TMaxRecordCount>
    requires RecordStructWithHasReady<TRecord>
class TaskCreatedResourceManager : public Manager<TRecord, TResourceRequest, TMaxRecordCount>
{
  public:
    Handle submit(device::StarDevice &device, job::TaskManager &taskSystem, system::EventBus &eventBus,
                  TResourceRequest request) override
    {
        auto handle = Manager<TRecord, TResourceRequest, TMaxRecordCount>::submit(device, taskSystem, eventBus,
                                                                                  std::move(request));

        TRecord *record = Manager<TRecord, TResourceRequest, TMaxRecordCount>::get(handle);
        submitTask(device, handle, taskSystem, eventBus, record);

        return handle;
    }

  protected:
    virtual Handle_Type getHandleType() const override = 0;

    virtual void submitTask(device::StarDevice &device, const Handle &handle, job::TaskManager &taskSystem,
                            system::EventBus &eventBus, TRecord *storedRecord) {};

    virtual TRecord createRecord(device::StarDevice &device, TResourceRequest &&request) const override = 0;
};
} // namespace star::core::device::manager