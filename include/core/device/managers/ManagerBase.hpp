#pragma once

#include "device/StarDevice.hpp"
#include "device/system/EventBus.hpp"
#include "job/TaskManager.hpp"

#include <starlight/common/Handle.hpp>
#include <starlight/common/HandleTypeRegistry.hpp>

#include <concepts>

namespace star::core::device::manager
{
template <typename T>
concept RecordStructHasCleanup = requires(T record, device::StarDevice &device) {
    { record.cleanupRender(device) } -> std::same_as<void>;
};

template <typename TRecord, typename TResourceRequest>
    requires RecordStructHasCleanup<TRecord>
class ManagerBase
{
  public:
    ManagerBase(std::string_view handleTypeName)
        : m_handleType(star::common::HandleTypeRegistry::instance().registerType(handleTypeName))
    {
    }
    virtual Handle submit(device::StarDevice &device, TResourceRequest resource) = 0;

    uint16_t &getHandleType()
    {
        return m_handleType;
    }
    uint16_t getHandleType() const
    {
        return m_handleType;
    }

  private:
    uint16_t m_handleType;
};
} // namespace star::core::device::manager