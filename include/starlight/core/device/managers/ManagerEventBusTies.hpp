#pragma once

#include "Manager.hpp"

#include "Enums.hpp"
#include "core/device/system/event/ManagerRequest.hpp"

#include <star_common/EventBus.hpp>

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

    ManagerEventBusTies(ManagerEventBusTies &&other) noexcept
        : Manager<TRecord, TResourceRequest, TMaxRecordCount>(std::move(other)), m_subscriberInfo(),
          m_eventBus(other.m_eventBus)
    {
        // Unsubscribe source BEFORE moving handle
        if (other.m_subscriberInfo.isInitialized() && other.m_eventBus)
        {
            other.unsubscribe(*other.m_eventBus);
        }

        // Now move the handle safely
        m_subscriberInfo = std::move(other.m_subscriberInfo);
        m_eventBus = other.m_eventBus;
        if (this->m_eventBus && this->m_device)
        {
            init(this->m_device, *this->m_eventBus);
        }
    }

    ManagerEventBusTies &operator=(ManagerEventBusTies &&other)
    {
        if (this != &other)
        {
            if (other.m_subscriberInfo.isInitialized() && other.m_eventBus)
            {
                other.unsubscribe(*other.m_eventBus);
            }

            Manager<TRecord, TResourceRequest, TMaxRecordCount>::operator=(std::move(other));
            m_eventBus = other.m_eventBus;
            m_registeredHandleEventType = std::move(other.m_registeredHandleEventType);
            m_subscriberInfo = std::move(other.m_subscriberInfo);

            if (other.m_device && other.m_eventBus)
            {
                init(other.m_device, *other.m_eventBus);
            }
        }
        return *this;
    }

    virtual void init(device::StarDevice *device, common::EventBus &eventBus)
    {
        Manager<TRecord, TResourceRequest, TMaxRecordCount>::init(device);

        submitSubscribeToEventBus(eventBus);
        m_eventBus = &eventBus;
    }

    void unsubscribe(common::EventBus &eventBus)
    {
        if (m_subscriberInfo.isInitialized())
        {
            eventBus.unsubscribe(m_subscriberInfo);

            m_subscriberInfo = Handle();
        }
    }

    virtual void cleanupRender()
    {
        assert(m_eventBus);

        unsubscribe(*m_eventBus);
        this->Manager<TRecord, TResourceRequest, TMaxRecordCount>::cleanupRender();
    }

  protected:
    Handle m_subscriberInfo;
    common::EventBus *m_eventBus = nullptr;

    void eventCallback(const star::common::IEvent &e, bool &keepAlive)
    {
        const auto &requestEvent =
            static_cast<const core::device::system::event::ManagerRequest<TResourceRequest> &>(e);
        auto resultHandle = this->submit(requestEvent.giveMeRequest());

        requestEvent.getResultingHandle() = resultHandle;
        if (auto out = requestEvent.getResultingResourcePointer())
        {
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

    void submitSubscribeToEventBus(common::EventBus &bus)
    {
        bus.subscribe(m_registeredHandleEventType,
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