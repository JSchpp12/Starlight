#pragma once

#include "Handle.hpp"
#include "device/StarDevice.hpp"
#include "device/system/EventBus.hpp"
#include "job/TaskManager.hpp"

#include <concepts>

namespace star::core::device::manager
{
template <typename T>
concept RecordStructHasCleanup = requires(T record, device::StarDevice &device) {
    { record.cleanupRender(device) } -> std::same_as<void>;
};

template <typename TRecord, typename TResourceRequest, star::Handle_Type THandleType>
    requires RecordStructHasCleanup<TRecord>
class ManagerBase
{
  public:
    virtual Handle submit(device::StarDevice &device, TResourceRequest resource) = 0;
};
} // namespace star::core::device::manager