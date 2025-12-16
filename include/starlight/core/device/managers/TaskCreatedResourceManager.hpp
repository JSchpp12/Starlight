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
    TaskCreatedResourceManager(TaskCreatedResourceManager &&other) noexcept = default;
    TaskCreatedResourceManager &operator=(TaskCreatedResourceManager &&other) noexcept = default;

    virtual Handle submit(TResourceRequest request) override
    {
        assert(this->m_device != nullptr && this->m_deviceTaskSystem != nullptr && this->m_deviceEventBus != nullptr &&
               "Device or Task System not initialized");

        auto handle = this->ManagerEventBusTies<TRecord, TResourceRequest, TMaxRecordCount>::submit(std::move(request));

        TRecord *record = this->get(handle);
        submitTask(*this->m_device, handle, *this->m_deviceTaskSystem, *this->m_deviceEventBus, record);

        return handle;
    }

    virtual void init(device::StarDevice *device, common::EventBus &eventBus, job::TaskManager &taskSystem)
    {
        this->ManagerEventBusTies<TRecord, TResourceRequest, TMaxRecordCount>::init(device, eventBus);

        m_deviceTaskSystem = &taskSystem;
    }

  protected:
    using Manager<TRecord, TResourceRequest, TMaxRecordCount>::submit;

    job::TaskManager *m_deviceTaskSystem = nullptr;

    virtual void submitTask(device::StarDevice &device, const Handle &handle, job::TaskManager &taskSystem,
                            common::EventBus &eventBus, TRecord *storedRecord) = 0;

    virtual TRecord createRecord(TResourceRequest &&request) const override = 0;

  private:
    using ManagerEventBusTies<TRecord, TResourceRequest, TMaxRecordCount>::init;
};
} // namespace star::core::device::manager