#pragma once

#include "starlight/enums/Enums.hpp"

#include <star_common/Handle.hpp>
#include <star_common/IEvent.hpp>

#include <vulkan/vulkan.hpp>

#include <string_view>

namespace star::event
{
constexpr std::string_view GetQueueEventTypeName = "star::event::GetQueue";

class GetQueue : public common::IEvent
{
  public:
    class Builder
    {
      public:
        Builder() = default;
        Builder &setQueueData(Handle &queueData); 
        Builder &setQueueType(const star::Queue_Type &type);
        Builder &setSelectFromFamilyIndex(const std::unordered_set<uint8_t> &familyIndex);
        Builder &setAvoidFamilyIndex(const std::unordered_set<uint8_t> &familyIndex);
        Builder &getEngineDedicatedQueue();
        GetQueue build();

      private:
        star::Queue_Type m_type;
        bool m_selectEngineQueue = false;
        Handle *m_queueData = nullptr;
        const std::unordered_set<uint8_t> *m_selectFromFamilyIndex = nullptr;
        const std::unordered_set<uint8_t> *m_avoidFamilyIndex = nullptr;
    };
    virtual ~GetQueue() = default;

    Handle *getQueueResultLocation() const
    {
        return m_queueData;
    }
    const star::Queue_Type &getRequestedQueueType() const
    {
        return m_requestedQueueType;
    }
    const std::unordered_set<uint8_t> *getQueueFamilyInfo() const
    {
        return m_selectedQueueFamilyIndexInfo;
    }
    bool getRequestEngineReservedQueue() const
    {
        return m_requestEngineReservedQueue;
    }
    bool getSelectFromIndex() const
    {
        return m_selectFromIndex;
    }
    bool getAvoidIndex() const
    {
        return m_avoidIndex;
    }

  private:
    friend class Builder;

    GetQueue(Handle *queueData, star::Queue_Type requestedQueueType, bool getEngineReservedQueue);
    GetQueue(Handle *queueData, star::Queue_Type requestedQueueType, bool getEngineReservedQueue, const std::unordered_set<uint8_t> &familyIndex,
             bool selectFromIndex, bool avoidIndex); 

    mutable Handle *m_queueData = nullptr;
    star::Queue_Type m_requestedQueueType;
    bool m_requestEngineReservedQueue;
    const std::unordered_set<uint8_t> *m_selectedQueueFamilyIndexInfo = nullptr;
    bool m_selectFromIndex = false;
    bool m_avoidIndex = false; 
};
} // namespace star::event