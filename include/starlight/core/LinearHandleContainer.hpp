#pragma once

#include "Enums.hpp"
#include "HandleContainer.hpp"
#include "core/Exceptions.hpp"
#include "device/StarDevice.hpp"

#include <array>
#include <stack>
#include <star_common/Handle.hpp>
#include <star_common/helper/CastHelpers.hpp>

namespace star::core
{

template <typename TData, size_t TMaxDataCount> class LinearHandleContainer : public HandleContainer<TData>
{
  public:
    LinearHandleContainer(std::string_view handleTypeName) : HandleContainer<TData>(handleTypeName)
    {
    }
    LinearHandleContainer(uint16_t registeredHandleType) : HandleContainer<TData>(std::move(registeredHandleType))
    {
    }
    virtual ~LinearHandleContainer() = default;

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
        const uint32_t acqSpace = this->getNextSpace();

        const Handle newHandle = Handle{.type = this->getHandleType(), .id = acqSpace};

        m_records[static_cast<const size_t &>(acqSpace)] = std::move(newData);

        return newHandle;
    }

    uint32_t getNextSpace()
    {
        if (!m_skippedSpaces.empty())
        {
            const uint32_t id = m_skippedSpaces.top();
            m_skippedSpaces.pop();
            return id;
        }

        if (m_nextSpace >= m_records.size())
        {
            STAR_THROW("Storage is full");
        }

        return m_nextSpace++;
    }

    TData &getRecord(const Handle &handle) override
    {
        assert(handle.getID() < m_records.size() && "Handle references location outside of available storage");
        size_t index = 0;
        star::common::helper::SafeCast<uint32_t, size_t>(handle.getID(), index);

        return m_records[index];
    }

    const TData &getRecord(const Handle &handle) const override
    {
        assert(handle.getID() < m_records.size() && "Handle references location outside of available storage");
        size_t index = 0;
        star::common::helper::SafeCast<uint32_t, size_t>(handle.getID(), index);

        return m_records[index];
    }

    virtual void removeRecord(const Handle &handle, device::StarDevice *device) override
    {
        (void)device;

        assert(handle.getID() < m_records.size() && "Requested index is beyond max storage space in remove()");
        m_skippedSpaces.push(handle.getID());
    }
};
} // namespace star::core