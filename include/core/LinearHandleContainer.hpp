#pragma once

#include "CastHelpers.hpp"
#include "Enums.hpp"
#include "Handle.hpp"
#include "HandleContainer.hpp"
#include "device/StarDevice.hpp"

#include <array>
#include <stack>

namespace star::core
{

template <typename TData, star::Handle_Type THandleType, size_t TMaxDataCount>
class LinearHandleContainer : public HandleContainer<TData, THandleType>
{
  public:
    std::array<TData, TMaxDataCount> &getData()
    {
        return m_records;
    }

  protected:
    std::stack<uint32_t> m_skippedSpaces = std::stack<uint32_t>();
    std::array<TData, TMaxDataCount> m_records = std::array<TData, TMaxDataCount>();
    uint32_t m_nextSpace = 0;

    Handle storeRecord(TData newData) override
    {
        uint32_t nextSpace = getNextSpace();
        Handle newHandle = Handle{.type = THandleType, .id = nextSpace};

        m_records[nextSpace] = std::move(newData);

        return newHandle;
    }

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

    TData &getRecord(const Handle &handle) override
    {
        assert(handle.getID() < m_records.size() && "Handle references location outside of available storage");
        size_t index = 0;
        CastHelpers::SafeCast<uint32_t, size_t>(handle.getID(), index);

        return m_records[index];
    }

    virtual void removeRecord(const Handle &handle, device::StarDevice *device) override
    {
        assert(handle.getID() < m_records.size() && "Requested index is beyond max storage space in remove()");

        m_skippedSpaces.push(handle.getID());
    }
};
} // namespace star::core