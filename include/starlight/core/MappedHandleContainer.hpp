#pragma once

#include "HandleContainer.hpp"

#include <star_common/helper/CastHelpers.hpp>

#include <absl/container/flat_hash_map.h>

namespace star::core
{
template <typename TData> class MappedHandleContainer : public HandleContainer<TData>
{
  public:
    MappedHandleContainer(std::string_view handleTypeName) : HandleContainer<TData>(handleTypeName)
    {
    }
    void manualInsert(const Handle &handle, TData record)
    {
        store(handle, std::move(record));
    }

    bool contains(const Handle &handle) const
    {
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
    absl::flat_hash_map<Handle, TData, star::HandleHash> m_records;

    Handle createNewHandle() const
    {
        uint32_t newId = 0;
        common::helper::SafeCast<size_t, uint32_t>(m_records.size(), newId);

        Handle newHandle{.type = this->getHandleType(), .id = std::move(newId)};

        return newHandle;
    }

    void store(const Handle &recordHandle, TData record)
    {
        assert(recordHandle.getType() == this->getHandleType() && "Ensure proper handle type for container");
        if (m_records.contains(recordHandle)){
            m_records.find(recordHandle)->second = std::move(record);
        }else{
            m_records.insert(std::make_pair(recordHandle, std::move(record)));
        }
    }
};
} // namespace star::core