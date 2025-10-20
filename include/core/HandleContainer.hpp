#pragma once

#include "Enums.hpp"
#include "Handle.hpp"
#include "core/device/StarDevice.hpp"

#include <cassert>

namespace star::core
{
template <typename TData, star::Handle_Type THandleType> class HandleContainer
{
  public:
    Handle insert(TData newData)
    {
        return storeRecord(std::move(newData));
    }
    TData &get(const Handle &handle)
    {
        assert(handle.getType() == THandleType && "Ensure proper handle type provided");
        return getRecord(handle);
    }
    void remove(const Handle &handle, device::StarDevice *device = nullptr)
    {
        assert(handle.getType() == THandleType && "Ensure proper handle type provided");
        removeRecord(handle, device);
    }

  protected:
    virtual Handle storeRecord(TData newData) = 0;
    virtual TData &getRecord(const Handle &handle) = 0;
    virtual void removeRecord(const Handle &handle, device::StarDevice *device = nullptr) = 0;
};
} // namespace star::core