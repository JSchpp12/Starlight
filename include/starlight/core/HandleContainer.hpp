#pragma once

#include "core/device/StarDevice.hpp"

#include <star_common/Handle.hpp>
#include <star_common/HandleTypeRegistry.hpp>

#include <cassert>

namespace star::core
{
template <typename TData> class HandleContainer
{
  public:
    HandleContainer(std::string_view handleTypeName)
        : m_handleType(common::HandleTypeRegistry::instance().registerType(handleTypeName))
    {
    }
    HandleContainer(uint16_t handleType) : m_handleType(std::move(handleType))
    {
    }

    Handle insert(TData newData)
    {
        return storeRecord(std::move(newData));
    }
    TData &get(const Handle &handle)
    {
        assert(handle.getType() == m_handleType && "Ensure proper handle type provided");
        return getRecord(handle);
    }
    const TData &get(const Handle &handle) const
    {
        assert(handle.getType() == m_handleType && "Ensure proper handle type provided");
        return getRecord(handle);
    }
    void remove(const Handle &handle, device::StarDevice *device = nullptr)
    {
        assert(handle.getType() == m_handleType && "Ensure proper handle type provided");
        removeRecord(handle, device);
    }
    uint16_t &getHandleType()
    {
        return m_handleType;
    }
    uint16_t getHandleType() const
    {
        return m_handleType;
    }

  protected:
    virtual Handle storeRecord(TData newData) = 0;
    virtual TData &getRecord(const Handle &handle) = 0;
    virtual void removeRecord(const Handle &handle, device::StarDevice *device = nullptr) = 0;

  private:
    uint16_t m_handleType;
};
} // namespace star::core