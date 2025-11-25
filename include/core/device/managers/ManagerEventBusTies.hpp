#pragma once

#include "Manager.hpp"

#include "Enums.hpp"
#include "core/device/system/EventBus.hpp"
#include "core/device/system/event/ManagerRequest.hpp"

namespace star::core::device::manager
{
template <typename TRecord, typename TResourceRequest, size_t TMaxRecordCount>
class ManagerEventBusTies : public Manager<TRecord, TResourceRequest, TMaxRecordCount>
{
  public:
    ManagerEventBusTies(std::string_view handleTypeName, const std::string_view eventTypeName)
        : Manager<TRecord, TResourceRequest, TMaxRecordCount>(handleTypeName),
          m_registeredHandleEventType(common::HandleTypeRegistry::instance().registerType(eventTypeName))
    {
    }

    virtual ~ManagerEventBusTies() = default;

    ManagerEventBusTies(const ManagerEventBusTies &) = delete;
    ManagerEventBusTies &operator=(const ManagerEventBusTies &) = delete;
    ManagerEventBusTies(ManagerEventBusTies &&) = delete;
    ManagerEventBusTies &operator=(ManagerEventBusTies &&) = delete;

    void init(std::shared_ptr<device::StarDevice> device, core::device::system::EventBus &bus) override
    {
        m_device = device;

        submitSubscribeToEventBus(bus);
    }

    virtual void cleanup(core::device::system::EventBus &bus)
    {
        assert(m_subscriberInfo.isInitialized() && "Should not call cleanup twice");
        bus.unsubscribe(m_registeredHandleEventType, m_subscriberInfo);

        m_subscriberInfo = Handle();
    }

  protected:
    std::shared_ptr<device::StarDevice> m_device;
    Handle m_subscriberInfo;

    void eventCallback(const star::common::IEvent &e, bool &keepAlive)
    {
        const auto &requestEvent =
            static_cast<const core::device::system::event::ManagerRequest<TResourceRequest> &>(e);
        auto resultHandle = this->submit(*this->m_device, requestEvent.giveMeRequest());

        requestEvent.getResultingHandle() = resultHandle;
        if (auto out = requestEvent.getResultingResourcePointer()){
            void *record = static_cast<void *>(this->get(resultHandle));
            **out = record;
        }
        
        keepAlive = true;
    }

    Handle *getHandleForUpdate()
    {
        return &m_subscriberInfo;
    }

    void notificationFromEventBusHandleDelete(const Handle &noLongerNeededSubscriberHandle)
    {
        if (this->m_subscriberInfo == noLongerNeededSubscriberHandle)
        {
            this->m_subscriberInfo = Handle();
        }
    }

    void submitSubscribeToEventBus(core::device::system::EventBus &bus)
    {
        bus.subscribe(
            m_registeredHandleEventType, 
            {[this](const star::common::IEvent &e, bool &keepAlive) { this->eventCallback(e, keepAlive); },
             [this]() -> Handle * { return this->getHandleForUpdate(); },
             [this](const Handle &noLongerNeededHandle) {
                 this->notificationFromEventBusHandleDelete(noLongerNeededHandle);
             }});
    };

  private:
    uint16_t m_registeredHandleEventType;
};
} // namespace star::core::device::manager