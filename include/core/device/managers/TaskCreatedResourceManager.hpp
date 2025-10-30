#pragma once

#include "ManagerEventBusTies.hpp"

namespace star::core::device::manager
{
template <typename T>
concept RecordStructWithHasReady = requires(const T record) {
    { record.isReady() } -> std::same_as<bool>;
};
template <typename TRecord, typename TResourceRequest, star::Handle_Type THandleType, size_t TMaxRecordCount>
    requires RecordStructWithHasReady<TRecord>
class TaskCreatedResourceManager : public ManagerEventBusTies<TRecord, TResourceRequest, THandleType, TMaxRecordCount>
{
  public:
    TaskCreatedResourceManager() = default;
    virtual ~TaskCreatedResourceManager() = default;
    TaskCreatedResourceManager(const TaskCreatedResourceManager &) noexcept = delete;
    TaskCreatedResourceManager &operator=(const TaskCreatedResourceManager &) noexcept = delete;
    TaskCreatedResourceManager(TaskCreatedResourceManager &&) noexcept = delete;
    TaskCreatedResourceManager &operator=(TaskCreatedResourceManager &&) noexcept = delete;

    virtual Handle submit(device::StarDevice &device, TResourceRequest request, job::TaskManager &taskSystem,
                          system::EventBus &eventBus)
    {
        auto handle =
            Manager<TRecord, TResourceRequest, THandleType, TMaxRecordCount>::submit(device, std::move(request));

        TRecord *record = Manager<TRecord, TResourceRequest, THandleType, TMaxRecordCount>::get(handle);
        submitTask(device, handle, taskSystem, eventBus, record);

        return handle;
    }

  protected:
    using Manager<TRecord, TResourceRequest, THandleType, TMaxRecordCount>::submit;
    virtual void submitTask(device::StarDevice &device, const Handle &handle, job::TaskManager &taskSystem,
                            system::EventBus &eventBus, TRecord *storedRecord) {};

    virtual TRecord createRecord(device::StarDevice &device, TResourceRequest &&request) const override = 0;
};
} // namespace star::core::device::manager