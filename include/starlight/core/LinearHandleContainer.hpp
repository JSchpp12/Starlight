#pragma once

#include "Enums.hpp"
#include "HandleContainer.hpp"
#include "core/Exceptions.hpp"
#include "device/StarDevice.hpp"

#include <stack>
#include <star_common/Handle.hpp>
#include <star_common/helper/CastHelpers.hpp>
#include <vector>

namespace star::core
{

template <typename TData> class LinearHandleContainer : public HandleContainer<TData>
{
  public:
    LinearHandleContainer(std::string_view handleTypeName, size_t startCapacity = 0, size_t expandingAmt = 0)
        : HandleContainer<TData>(handleTypeName)
    {
        m_records.reserve(startCapacity);
        m_filled.reserve(startCapacity);
        m_expandingAmt = expandingAmt;
    }
    LinearHandleContainer(uint16_t registeredHandleType, size_t startCapacity = 0, size_t expandingAmt = 0)
        : HandleContainer<TData>(std::move(registeredHandleType))
    {
        m_records.reserve(startCapacity);
        m_filled.reserve(startCapacity);
        m_expandingAmt = expandingAmt;
    }
    virtual ~LinearHandleContainer() = default;

    std::vector<TData> &getData()
    {
        return m_records;
    }

    /// Reserve a slot without storing data yet; the returned Handle is stable.
    /// The slot remains not-filled until commit() is invoked. Accessing a
    /// reserved-but-uncommitted slot via get()/getRecord() throws.
    Handle reserve()
    {
        const uint32_t acqSpace = getNextSpace();
        m_filled[static_cast<size_t>(acqSpace)] = false;
        return Handle{.type = this->getHandleType(), .id = acqSpace};
    }

    /// Whether the slot referenced by `handle` is currently filled (i.e. has
    /// been populated via insert()/commit() and not since removed).
    bool isFilled(const Handle &handle) const
    {
        if (handle.getID() >= m_records.size())
        {
            STAR_THROWF("isFilled: Handle references location outside of available storage: id=", handle.getID());
        }
        return m_filled[static_cast<size_t>(handle.getID())];
    }

    /// Fill a previously reserved slot with `data` and mark it as filled.
    void commit(const Handle &handle, TData data)
    {
        if (handle.getID() >= m_records.size())
        {
            STAR_THROWF("commit: Handle references location outside of available storage: id=", handle.getID());
        }
        const size_t index = static_cast<size_t>(handle.getID());
        m_records[index] = std::move(data);
        m_filled[index] = true;
    }

  protected:
    std::stack<uint32_t> m_skippedSpaces = std::stack<uint32_t>();
    std::vector<TData> m_records;
    std::vector<bool> m_filled;
    size_t m_expandingAmt = 0;
    uint32_t m_nextSpace = 0;

    Handle storeRecord(TData newData) override
    {
        const uint32_t acqSpace = this->getNextSpace();

        const Handle newHandle = Handle{.type = this->getHandleType(), .id = acqSpace};

        m_records[static_cast<const size_t &>(acqSpace)] = std::move(newData);
        m_filled[static_cast<const size_t &>(acqSpace)] = true;

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

        const uint32_t newId = m_nextSpace++;
        if (newId >= m_records.size())
        {
            if (m_expandingAmt > 0)
            {
                const size_t newSize = ((static_cast<size_t>(newId) / m_expandingAmt) + 1) * m_expandingAmt;
                m_records.resize(newSize);
                m_filled.resize(newSize);
            }
            else
            {
                m_records.resize(static_cast<size_t>(newId) + 1);
                m_filled.resize(static_cast<size_t>(newId) + 1);
            }
        }
        return newId;
    }

    TData &getRecord(const Handle &handle) override
    {
        if (handle.getID() >= m_records.size())
        {
            STAR_THROWF("Handle references location outside of available storage: id=", handle.getID());
        }
        size_t index = 0;
        star::common::casts::SafeCast<uint32_t, size_t>(handle.getID(), index);

        if (!m_filled[index])
        {
            STAR_THROWF("Handle references a slot that has not been filled: id=", handle.getID());
        }

        return m_records[index];
    }

    const TData &getRecord(const Handle &handle) const override
    {
        if (handle.getID() >= m_records.size())
        {
            STAR_THROWF("Handle references location outside of available storage: id=", handle.getID());
        }
        size_t index = 0;
        star::common::casts::SafeCast<uint32_t, size_t>(handle.getID(), index);

        if (!m_filled[index])
        {
            STAR_THROWF("Handle references a slot that has not been filled: id=", handle.getID());
        }

        return m_records[index];
    }

    virtual void removeRecord(const Handle &handle, device::StarDevice *device) override
    {
        (void)device;

        if (handle.getID() >= m_records.size())
        {
            STAR_THROWF("Requested index is beyond storage space in remove(): id=", handle.getID());
        }
        m_filled[static_cast<size_t>(handle.getID())] = false;
        m_skippedSpaces.push(handle.getID());
    }
};
} // namespace star::core