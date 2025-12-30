#pragma once

#include "starlight/event/FrameComplete.hpp"

#include <star_common/EventBus.hpp>
#include <star_common/Handle.hpp>
#include <star_common/HandleTypeRegistry.hpp>

#include <concepts>
#include <string_view>

namespace star::event
{
template <typename T>
concept ListenerLike = requires(T listener) {
    { listener.onFrameComplete() } -> std::same_as<void>;
};

template <typename TListener> class ListenForFrameCompletePolicy
{
  public:
    explicit ListenForFrameCompletePolicy(TListener &me)
        requires ListenerLike<TListener>
        : m_listenerHandle(), m_me(me) {};

    void init(common::EventBus &eventBus)
    {
        registerListener(eventBus);
    }

    void cleanup(common::EventBus &eventBus)
    {
    }

  private:
    Handle m_listenerHandle;
    TListener &m_me;

    Handle *getHandleForEventBus()
    {
        return &m_listenerHandle;
    }

    void registerListener(common::EventBus &eventBus)
    {
        eventBus.subscribe(common::HandleTypeRegistry::instance().registerType(GetFrameCompleteTypeName),
                           common::SubscriberCallbackInfo{
                               std::bind(&ListenForFrameCompletePolicy<TListener>::eventCallback, this,
                                         std::placeholders::_1, std::placeholders::_2),
                               std::bind(&ListenForFrameCompletePolicy<TListener>::getHandleForEventBus, this),
                               std::bind(&ListenForFrameCompletePolicy<TListener>::notificationFromEventBusOfDeletion,
                                         this, std::placeholders::_1)});
    }

    void eventCallback(const common::IEvent &e, bool &keepAlive)
    {
        const auto &event = static_cast<const event::FrameComplete &>(e);

        m_me.onFrameComplete();
        keepAlive = true;
    }

    void notificationFromEventBusOfDeletion(const Handle &noLongerNeededHandle)
    {
        if (m_listenerHandle == noLongerNeededHandle)
        {
            m_listenerHandle = Handle();
        }
    }
};
} // namespace star::event