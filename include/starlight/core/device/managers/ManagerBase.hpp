#pragma once

#include "device/StarDevice.hpp"
#include "job/TaskManager.hpp"

#include <star_common/EventBus.hpp>
#include <star_common/Handle.hpp>
#include <star_common/HandleTypeRegistry.hpp>

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
    ManagerBase(const ManagerBase &) = delete;
    ManagerBase &operator=(const ManagerBase &) = delete;
    ManagerBase(ManagerBase &&other) noexcept : m_device(other.m_device)
    {
    }
    ManagerBase &operator=(ManagerBase &&other) noexcept
    {
        if (this != &other)
        {
            m_device = other.m_device;
        }
        return *this;
    }
    virtual ~ManagerBase() = default;

    virtual Handle submit(TResourceRequest resource) = 0;

    void init(device::StarDevice *device)
    {
        m_device = device;
    }
    uint16_t &getHandleType()
    {
        return m_handleType;
    }
    const uint16_t &getHandleType() const
    {
        return m_handleType;
    }

  protected:
    core::device::StarDevice *m_device = nullptr;

  private:
    uint16_t m_handleType;
};
} // namespace star::core::device::manager