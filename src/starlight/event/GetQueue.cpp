#include "starlight/event/GetQueue.hpp"

#include <star_common/HandleTypeRegistry.hpp>

namespace star::event
{
GetQueue::GetQueue(Handle *queueData, star::Queue_Type requestedQueueType, bool getEngineReservedQueue)
    : IEvent(common::HandleTypeRegistry::instance().registerType(GetQueueEventTypeName)), m_queueData(queueData),
      m_requestedQueueType(std::move(requestedQueueType)), m_requestEngineReservedQueue(getEngineReservedQueue)
{
}

GetQueue::GetQueue(Handle *queueData, star::Queue_Type requestedQueueType, bool getEngineReservedQueue,
                   const std::unordered_set<uint8_t> &familyIndex, bool selectFromIndex, bool avoidIndex)
    : IEvent(common::HandleTypeRegistry::instance().registerType(GetQueueEventTypeName)), m_queueData(queueData),
      m_requestedQueueType(std::move(requestedQueueType)), m_requestEngineReservedQueue(getEngineReservedQueue),
      m_selectedQueueFamilyIndexInfo(&familyIndex), m_selectFromIndex(selectFromIndex), m_avoidIndex(avoidIndex)
{
}

GetQueue::Builder &GetQueue::Builder::setQueueData(Handle &queueData)
{
    m_queueData = &queueData;
    return *this;
}

GetQueue::Builder &GetQueue::Builder::setQueueType(const star::Queue_Type &type)
{
    m_type = type;
    return *this;
}

GetQueue::Builder &GetQueue::Builder::setSelectFromFamilyIndex(const std::unordered_set<uint8_t> &familyIndex)
{
    m_selectFromFamilyIndex = &familyIndex;
    return *this;
}

GetQueue::Builder &GetQueue::Builder::setAvoidFamilyIndex(const std::unordered_set<uint8_t> &familyIndex)
{
    m_avoidFamilyIndex = &familyIndex;
    return *this;
}

GetQueue::Builder &GetQueue::Builder::getEngineDedicatedQueue()
{
    m_selectEngineQueue = true;
    return *this;
}

GetQueue GetQueue::Builder::build()
{
    assert(((m_selectFromFamilyIndex != nullptr && m_avoidFamilyIndex == nullptr ||
             m_selectFromFamilyIndex == nullptr && m_avoidFamilyIndex != nullptr) || 
           (m_selectFromFamilyIndex == nullptr && m_avoidFamilyIndex == nullptr)) &&
           "Can only select from or avoid a family index. Combination of both is not supported yet");
    assert(m_queueData != nullptr && "Queue data must be provided to builder before build()");

    if (m_selectFromFamilyIndex != nullptr)
    {
        return GetQueue(m_queueData, m_type, m_selectEngineQueue, *m_selectFromFamilyIndex, true, false);
    }
    else if (m_avoidFamilyIndex != nullptr)
    {
        return GetQueue(m_queueData, m_type, m_selectEngineQueue, *m_avoidFamilyIndex, false, true);
    }

    return GetQueue(m_queueData, m_type, m_selectEngineQueue);
}

} // namespace star::event