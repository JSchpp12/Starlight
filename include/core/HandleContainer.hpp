#pragma once

#include "CastHelpers.hpp"
#include "Enums.hpp"
#include "Handle.hpp"
#include "device/StarDevice.hpp"

#include <array>
#include <stack>

namespace star::core
{
template <typename T>
concept TDataHasCleanupRender = requires(T record, device::StarDevice &device) {
    { record.cleanupRender(device) } -> std::same_as<void>;
};
template <typename T>
concept TDataHasCleanup = requires(T record){
    {record.cleanup()} -> std::same_as<void>;
};

template <typename T>
concept TDataHasProperCleanup = TDataHasCleanupRender<T> || TDataHasCleanup<T>;

template <typename TData, star::Handle_Type THandleType, size_t TMaxDataCount>
    requires TDataHasProperCleanup<TData>
class HandleContainer
{
  public:
    Handle insert(TData newData)
    {
        uint32_t nextSpace = getNextSpace();
        Handle newHandle = Handle{.type = THandleType, .id = nextSpace};

        m_records[nextSpace] = std::move(newData);

        return newHandle;
    }

    TData &get(const Handle &handle)
    {
        assert(handle.getID() < m_records.size() && "Handle references location outside of available storage");
        assert(handle.getType() == THandleType && "Invalid handle type provided");

        size_t index = 0;
        CastHelpers::SafeCast<uint32_t, size_t>(handle.getID(), index);

        return m_records[index];
    }

    std::array<TData, TMaxDataCount> &getData()
    {
        return m_records;
    }

    void cleanupAll(device::StarDevice *device = nullptr){
        for (uint32_t i = 0; i< m_records.size(); i++){
            auto handle = Handle{
                .type = THandleType,
                .id = i
            };
            cleanup(handle, device); 
        }
    }

    void remove(const Handle &handle, device::StarDevice *device = nullptr){
        assert(handle.getID() < m_records.size() && "Requested index is beyond max storage space in remove()"); 

        cleanup(handle, device);
        m_skippedSpaces.push(handle.getID()); 
    }

  private:
    std::stack<uint32_t> m_skippedSpaces = std::stack<uint32_t>();
    std::array<TData, TMaxDataCount> m_records = std::array<TData, TMaxDataCount>();
    uint32_t m_nextSpace = 0;

    uint32_t getNextSpace()
    {
        uint32_t nextSpace = 0;

        if (m_nextSpace > m_records.size())
        {
            if (m_skippedSpaces.empty())
            {
                throw std::runtime_error("Storage is full");
            }
            else
            {
                nextSpace = m_skippedSpaces.top();
                m_skippedSpaces.pop();
            }
        }
        else
        {
            nextSpace = m_nextSpace;
            m_nextSpace++;
        }

        return nextSpace;
    }

    void cleanup(const Handle &handle, device::StarDevice *device = nullptr){
        assert(handle.getID() < m_records.size() && "Handle references location outside of available storage in cleanup"); 

        if constexpr(TDataHasCleanupRender<TData>){
            assert(device != nullptr && "Device must be provided for types which require it in their cleanup"); 
            m_records[handle.getID()].cleanupRender(*device); 
        } else if constexpr(TDataHasCleanup<TData>){
            m_records[handle.getID()].cleanup();
        }
    }
};
} // namespace star::core