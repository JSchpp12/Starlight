#pragma once

#include "CastHelpers.hpp"
#include "Enums.hpp"
#include "HandleContainer.hpp"
#include "device/StarDevice.hpp"
#include <starlight/common/Handle.hpp>

#include <array>
#include <vector>

namespace star::core
{

template <typename TData, size_t TMaxDataCount> class LinearHandleContainer : public HandleContainer<TData>
{
  public:
    LinearHandleContainer(const std::string &handleTypeName) : HandleContainer<TData>(handleTypeName)
    {
    }
    LinearHandleContainer(uint16_t registeredHandleType) : HandleContainer<TData>(std::move(registeredHandleType))
    {
    }
    std::array<TData, TMaxDataCount> &getData()
    {
        return m_records;
    }

  protected:
    std::vector<uint32_t> m_skippedSpaces = std::vector<uint32_t>();
    std::array<TData, TMaxDataCount> m_records = std::array<TData, TMaxDataCount>();
    uint32_t m_nextSpace = 0;

    Handle storeRecord(TData newData) override
    {
        const uint32_t nextSpace = getNextSpace();
        const Handle newHandle = Handle{.type = this->getHandleType(), .id = nextSpace};

        m_records[nextSpace] = std::move(newData);

        return newHandle;
    }

    uint32_t getNextSpace()
    {
        if (!m_skippedSpaces.empty())
        {
            const uint32_t id = m_skippedSpaces.back();
            m_skippedSpaces.pop_back();
            return id;
        }

        if (m_nextSpace >= m_records.size())
        {
            throw std::runtime_error("Storage is full");
        }

        return m_nextSpace++;
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

        m_skippedSpaces.push_back(handle.getID());
    }
};
} // namespace star::core