#pragma once

#include "starlight/policy/event/ListenFor.hpp"

namespace star::core::waiter::one_shot
{
template <typename T, typename TEvent>
using ListenForEvent = star::policy::event::ListenFor<T, TEvent, TEvent::GetUniqueTypeName, &T::onEvent>;

template <typename T, typename TEvent> class GenericEvent : public std::enable_shared_from_this<GenericEvent<T, TEvent>>
{
  public:
    friend class Builder;

    class Builder
    {
      public:
        explicit Builder(star::common::EventBus &evtBus) : m_payload(), m_evtBus(evtBus)
        {
        }
        Builder &setPayload(T payload)
        {
            m_payload = std::move(payload);
            return *this;
        }
        void build()
        {
            assert(m_payload.has_value());
            auto shared =
                std::shared_ptr<GenericEvent<T, TEvent>>(new GenericEvent<T, TEvent>(std::move(m_payload.value())));
            shared->registerListener(m_evtBus);
        }

      private:
        star::common::EventBus &m_evtBus;
        std::optional<T> m_payload{std::nullopt};
    };

    /// <summary>
    /// Captures shared_from_this() into the event bus callback.
    /// The event bus is the sole owner after this call.
    /// When the callback sets keepAlive = false, the lambda is
    /// destroyed, the shared_ptr drops, and this object is deleted.
    /// </summary>
    /// <param name="eBus">Reference to the event bus object</param>
    void registerListener(star::common::EventBus &eBus)
    {
        auto self = this->shared_from_this();
        auto type = common::HandleTypeRegistry::instance().registerType(TEvent::GetUniqueTypeName());

        eBus.subscribe(type, star::common::SubscriberCallbackInfo{
                                 [self](const common::IEvent &e, bool &keepAlive) { self->onEvent(e, keepAlive); },
                                 [self]() -> Handle * { return &self->m_registration; },
                                 [self](const Handle &h) {
                                     if (self->m_registration == h)
                                     {
                                         self->m_registration = Handle();
                                     }
                                 }});
    }

    void onEvent(const star::common::IEvent &e, bool &keepAlive)
    {
        const auto &event = static_cast<const TEvent &>(e);
        m_payload(event, keepAlive);
    }

  private:
    T m_payload;
    Handle m_registration;

    explicit GenericEvent(T payload) : m_payload(std::move(payload))
    {
    }
};
} // namespace star::core::waiter::one_shot