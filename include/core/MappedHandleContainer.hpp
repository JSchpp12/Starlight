#pragma once

#include "CastHelpers.hpp"
#include "HandleContainer.hpp"

#include <unordered_map>

namespace star::core
{
template <typename TData, star::Handle_Type THandleType>
class MappedHandleContainer : public HandleContainer<TData, THandleType>
{
  public:
    void manualInsert(const Handle &handle, TData record)
    {
        store(handle, std::move(record));
    }

    bool contains(const Handle &handle) const{
        return m_records.contains(handle);
    }

  protected:
    Handle storeRecord(TData newData) override
    {
        Handle insertHandle = createNewHandle();
        store(insertHandle, std::move(newData));

        return insertHandle;
    }

    TData &getRecord(const Handle &handle) override
    {
        assert(m_records.contains(handle) && "Handle must be in the set");
        return m_records[handle];
    }
    void removeRecord(const Handle &handle, device::StarDevice *device) override
    {
        assert(m_records.contains(handle) && "Handle must be in the set");

        m_records.erase(handle);
    }

  private:
    std::unordered_map<Handle, TData, star::HandleHash> m_records;

    Handle createNewHandle() const
    {
        uint32_t newId = 0;
        CastHelpers::SafeCast<size_t, uint32_t>(m_records.size(), newId);

        Handle newHandle{.type = THandleType, .id = std::move(newId)};

        return newHandle;
    }

    void store(const Handle &recordHandle, TData record)
    {
        assert(recordHandle.getType() == THandleType && "Ensure proper handle type for container");
        assert(!m_records.contains(recordHandle) && "Handle must be unique and not already used");

        m_records.insert(std::make_pair(recordHandle, std::move(record)));
    }
};
} // namespace star::core