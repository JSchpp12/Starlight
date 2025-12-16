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
        : Manager<TRecord, TResourceRequest, TMaxRecordCount>(std::move(other)),
          m_subscriberInfo(), // don't move this; resubscribe below
          m_registeredHandleEventType(other.m_registeredHandleEventType)
    {
        if (other.m_deviceEventBus && other.m_device)
        {
            // Unsubscribe source to avoid dangling callbacks
            if (other.m_subscriberInfo.isInitialized())
            {
                other.cleanupRender();
            }
            // Initialize/resubscribe in our new 'this'
            init(other.m_device, *other.m_deviceEventBus);
        }
    }

    ManagerEventBusTies &operator=(ManagerEventBusTies &&other)
    {
        if (this != &other)
        {
            // Clean up existing subscription
            if (m_subscriberInfo.isInitialized() && this->m_deviceEventBus)
            {
                this->m_deviceEventBus->unsubscribe(m_subscriberInfo);
            }

            // Move base class
            Manager<TRecord, TResourceRequest, TMaxRecordCount>::operator=(std::move(other));

            // Move member variables
            m_registeredHandleEventType = std::move(other.m_registeredHandleEventType);
            m_subscriberInfo = std::move(other.m_subscriberInfo);

            // Reinitialize if needed
            if (other.m_device && other.m_deviceEventBus)
            {
                init(other.m_device, *other.m_deviceEventBus);
            }
        }
        return *this;
    }

    virtual void init(device::StarDevice *device, common::EventBus &eventBus) override
    {
        Manager<TRecord, TResourceRequest, TMaxRecordCount>::init(device, eventBus);

        submitSubscribeToEventBus(eventBus);
    }

    virtual void cleanupRender() override
    {
        assert(m_subscriberInfo.isInitialized() && "Should not call cleanup twice");
        this->m_deviceEventBus->unsubscribe(m_subscriberInfo);

        m_subscriberInfo = Handle();

        this->Manager<TRecord, TResourceRequest, TMaxRecordCount>::cleanupRender();
    }

  protected:
    Handle m_subscriberInfo;

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