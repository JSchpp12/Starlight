#pragma once

#include "starlight/event/GetQueue.hpp"

#include <concepts>
#include <star_common/EventBus.hpp>

namespace star::policy
{
template <typename T>
concept OnGetQueueListenerLike = requires(T listener, const event::GetQueue &event, bool &keepAlive) {
    { listener.onGetQueue(event, keepAlive) } -> std::same_as<void>;
};
template <typename T> class ListenForGetQueuePolicy
{
  public:
    explicit ListenForGetQueuePolicy(T &me) : m_listenerHandle(), m_me(me)
    {
    }

    void init(common::EventBus &eventBus)
    {
        registerListener(eventBus);
    }

    void cleanup(common::EventBus &eventBus)
    {
        if (m_listenerHandle.isInitialized())
        {
            eventBus.unsubscribe(m_listenerHandle);
        }
    }

  private:
    Handle m_listenerHandle;
    T &m_me;

    Handle *getHandleForEventBus()
    {
        return &m_listenerHandle;
    }

    void registerListener(common::EventBus &eventBus)
    {
        eventBus.subscribe(
            common::HandleTypeRegistry::instance().registerType(event::GetQueueEventTypeName),
            common::SubscriberCallbackInfo{std::bind(&ListenForGetQueuePolicy<T>::eventCallback, this,
                                                     std::placeholders::_1, std::placeholders::_2),
                                           std::bind(&ListenForGetQueuePolicy<T>::getHandleForEventBus, this),
                                           std::bind(&ListenForGetQueuePolicy<T>::notificationFromEventBusOfDeletion,
                                                     this, std::placeholders::_1)});
    }

    void eventCallback(const common::IEvent &e, bool &keepAlive)
        requires OnGetQueueListenerLike<T>
    {
        const auto &event = static_cast<const event::GetQueue &>(e);

        m_me.onGetQueue(event, keepAlive);
    }

    void notificationFromEventBusOfDeletion(const Handle &noLongerNeededHandle)
    {
        if (m_listenerHandle == noLongerNeededHandle)
        {
            m_listenerHandle = Handle();
        }
    }
};
} // namespace star::policy