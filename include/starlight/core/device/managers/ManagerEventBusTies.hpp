#pragma once

#include "Manager.hpp"

#include "Enums.hpp"
#include "core/device/system/event/ManagerRequest.hpp"
#include "core/logging/LoggingFactory.hpp"
#include "event/ManagerReservedMemoryUpdate.hpp"

#include <star_common/EventBus.hpp>

#include <cstddef>
#include <utility>

namespace star::core::device::manager
{
template <typename TRecord, typename TResourceRequest, size_t TMaxRecordCount,
          ResourceTypeNameProvider TResourceTypeName>
class ManagerEventBusTies : public Manager<TRecord, TResourceRequest, TMaxRecordCount, TResourceTypeName>
{
  public:
    explicit ManagerEventBusTies(const std::string_view eventTypeName)
        : Manager<TRecord, TResourceRequest, TMaxRecordCount, TResourceTypeName>(),
          m_registeredHandleEventType(common::HandleTypeRegistry::instance().registerType(eventTypeName))
    {
    }
    virtual ~ManagerEventBusTies() = default;
    ManagerEventBusTies(const ManagerEventBusTies &) = delete;
    ManagerEventBusTies &operator=(const ManagerEventBusTies &) = delete;

    ManagerEventBusTies(ManagerEventBusTies &&other) noexcept
        : Manager<TRecord, TResourceRequest, TMaxRecordCount, TResourceTypeName>(std::move(other)), m_subscriberInfo(),
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

    ManagerEventBusTies &operator=(ManagerEventBusTies &&other) noexcept
    {
        if (this != &other)
        {
            if (other.m_subscriberInfo.isInitialized() && other.m_eventBus)
            {
                other.unsubscribe(*other.m_eventBus);
            }

            Manager<TRecord, TResourceRequest, TMaxRecordCount, TResourceTypeName>::operator=(std::move(other));
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

    /// @brief Submit a resource request and publish a reallocation event if the record storage moved.
    /// @details The event is emitted after the base insert, so the new slot is already filled and listeners
    /// can safely re-resolve every handle they hold. Emission is synchronous on the submitting thread; the
    /// engine currently submits on the main thread.
    virtual Handle submit(TResourceRequest resource) override
    {
        auto &storage = this->m_records.getData();
        auto *previousData = storage.data();
        const size_t previousCapacity = storage.capacity();

        Handle handle =
            Manager<TRecord, TResourceRequest, TMaxRecordCount, TResourceTypeName>::submit(std::move(resource));

        if (m_eventBus != nullptr && (storage.data() != previousData || storage.capacity() != previousCapacity))
        {
            star::core::logging::debug("ManagerReservedMemoryUpdate raised: handle type ", this->getHandleType(),
                                       ", capacity ", previousCapacity, " -> ", storage.capacity());
            m_eventBus->emit(
                star::event::ManagerReservedMemoryUpdate{this->getHandleType(), static_cast<void *>(this)});
        }

        return handle;
    }

    virtual void init(device::StarDevice *device, common::EventBus &eventBus)
    {
        init(device);

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

    virtual void cleanupRender() override
    {
        assert(m_eventBus);

        unsubscribe(*m_eventBus);
        this->Manager<TRecord, TResourceRequest, TMaxRecordCount, TResourceTypeName>::cleanupRender();
    }

  protected:
    using Manager<TRecord, TResourceRequest, TMaxRecordCount, TResourceTypeName>::init;

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