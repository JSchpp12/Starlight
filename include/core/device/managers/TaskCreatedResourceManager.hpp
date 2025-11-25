#pragma once

#include "ManagerEventBusTies.hpp"

namespace star::core::device::manager
{
template <typename T>
concept RecordStructWithHasReady = requires(const T record) {
    { record.isReady() } -> std::same_as<bool>;
};
template <typename TRecord, typename TResourceRequest, size_t TMaxRecordCount>
    requires RecordStructWithHasReady<TRecord>
class TaskCreatedResourceManager : public ManagerEventBusTies<TRecord, TResourceRequest, TMaxRecordCount>
{
  public:
    TaskCreatedResourceManager(std::string_view handleTypeName, std::string_view eventTypeName)
        : ManagerEventBusTies<TRecord, TResourceRequest, TMaxRecordCount>(handleTypeName, eventTypeName){};
    virtual ~TaskCreatedResourceManager() = default;
    TaskCreatedResourceManager(const TaskCreatedResourceManager &) noexcept = delete;
    TaskCreatedResourceManager &operator=(const TaskCreatedResourceManager &) noexcept = delete;
    TaskCreatedResourceManager(TaskCreatedResourceManager &&) noexcept = delete;
    TaskCreatedResourceManager &operator=(TaskCreatedResourceManager &&) noexcept = delete;

    virtual Handle submit(device::StarDevice &device, TResourceRequest request, job::TaskManager &taskSystem,
                          system::EventBus &eventBus)
    {
        auto handle = this->submit(device, std::move(request));

        TRecord *record = this->get(handle);
        submitTask(device, handle, taskSystem, eventBus, record);

        return handle;
    }

  protected:
    using Manager<TRecord, TResourceRequest, TMaxRecordCount>::submit;
    virtual void submitTask(device::StarDevice &device, const Handle &handle, job::TaskManager &taskSystem,
                            system::EventBus &eventBus, TRecord *storedRecord) {};

    virtual TRecord createRecord(device::StarDevice &device, TResourceRequest &&request) const override = 0;
};
} // namespace star::core::device::manager