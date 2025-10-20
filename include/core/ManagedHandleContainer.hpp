#pragma once

#include "LinearHandleContainer.hpp"

namespace star::core
{
template <typename T>
concept TDataHasCleanupRender = requires(T record, device::StarDevice &device) {
    { record.cleanupRender(device) } -> std::same_as<void>;
};
template <typename T>
concept TDataHasCleanup = requires(T record) {
    { record.cleanup() } -> std::same_as<void>;
};
template <typename T>
concept TDataHasProperCleanup = TDataHasCleanupRender<T> || TDataHasCleanup<T>;

template <typename TData, star::Handle_Type THandleType, size_t TMaxDataCount>
    requires TDataHasProperCleanup<TData>
class ManagedHandleContainer : public LinearHandleContainer<TData, THandleType, TMaxDataCount>
{
  public:
    void cleanupAll(device::StarDevice *device = nullptr)
    {
        for (uint32_t i = 0; i < this->m_records.size(); i++)
        {
            auto handle = Handle{.type = THandleType, .id = i};
            cleanup(handle, device);
        }
    }

  protected:
    void removeRecord(const Handle &handle, device::StarDevice *device = nullptr) override
    {
        LinearHandleContainer<TData, THandleType, TMaxDataCount>::remove(handle, device);

        cleanup(handle, device);
    }
    void cleanup(const Handle &handle, device::StarDevice *device = nullptr)
    {
        assert(handle.getID() < this->m_records.size() &&
               "Handle references location outside of available storage in cleanup");

        if constexpr (TDataHasCleanupRender<TData>)
        {
            assert(device != nullptr && "Device must be provided for types which require it in their cleanup");
            this->m_records[handle.getID()].cleanupRender(*device);
        }
        else if constexpr (TDataHasCleanup<TData>)
        {
            this->m_records[handle.getID()].cleanup();
        }
    }
};
} // namespace star::core