#pragma once

#include "HandleContainer.hpp"
#include "CastHelpers.hpp"

#include <unordered_map>

namespace star::core
{
template<typename TData, star::Handle_Type THandleType>
class MappedHandleContainer : public HandleContainer<TData, THandleType>
{
    public:
    void manualInsert(const Handle &handle, TData record){
        assert(handle.getType() == THandleType && "Ensure proper handle type for container"); 
        m_records.insert(std::make_pair(handle, std::move(record))); 
    }
    
    protected:
    Handle storeRecord(TData newData) override{
        uint32_t newId = 0;
        CastHelpers::SafeCast<size_t, uint32_t>(m_records.size(), newId); 

        Handle newHandle {
            .type = THandleType,
            .id = std::move(newId)
        };

        return newHandle;
    }

    TData &getRecord(const Handle &handle) override{
        assert(m_records.contains(handle) && "Handle must be in the set");
        return m_records[handle]; 
    }
    void removeRecord(const Handle &handle, device::StarDevice *device) override{
        assert(m_records.contains(handle) && "Handle must be in the set");

        m_records.erase(handle);
    }

    private:
    std::unordered_map<Handle, TData, star::HandleHash> m_records;
};
} // namespace star::core