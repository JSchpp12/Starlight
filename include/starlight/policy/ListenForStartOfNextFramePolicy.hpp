#pragma once

#include "starlight/event/StartOfNextFrame.hpp"
#include <star_common/EventBus.hpp>
#include <star_common/Handle.hpp>
#include <star_common/HandleTypeRegistry.hpp>

#include <concepts>

namespace star::policy
{
template <typename T>
concept ListenerForStartOfFrameLike =
    requires(T listener, const star::event::StartOfNextFrame &event, bool &keepAlive) {
        { listener.onStartOfNextFrame(event, keepAlive) } -> std::same_as<void>;
    };
template <typename T> class ListenForStartOfNextFramePolicy
{
  public:
    explicit ListenForStartOfNextFramePolicy(T &me) : m_listenerHandle(), m_me(me)
    {
    }
    ListenForStartOfNextFramePolicy(const ListenForStartOfNextFramePolicy &) = delete;
    ListenForStartOfNextFramePolicy &operator=(const ListenForStartOfNextFramePolicy &) = delete;
    ListenForStartOfNextFramePolicy(ListenForStartOfNextFramePolicy &&) = delete;
    ListenForStartOfNextFramePolicy &operator=(ListenForStartOfNextFramePolicy &&) = delete;
    ListenForStartOfNextFramePolicy() = default;

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
    Handle *getHandleForEventBus()
    {
        return &m_listenerHandle;
    }

    void notificationFromEventBusOfDeletion(const Handle &noLongerNeededHandle)
    {
        if (m_listenerHandle == noLongerNeededHandle)
        {
            m_listenerHandle = Handle();
        }
    }

    void eventCallback(const common::IEvent &e, bool &keepAlive)
        requires ListenerForStartOfFrameLike<T>
    {
        const auto &event = static_cast<const star::event::StartOfNextFrame &>(e);

        m_me.onStartOfNextFrame(event, keepAlive);
    }

    void registerListener(common::EventBus &eventBus)
    {
        eventBus.subscribe(
            common::HandleTypeRegistry::instance().registerType(star::event::GetStartOfNextFrameTypeName()),
            common::SubscriberCallbackInfo{
                std::bind(&ListenForStartOfNextFramePolicy<T>::eventCallback, this, std::placeholders::_1,
                          std::placeholders::_2),
                std::bind(&ListenForStartOfNextFramePolicy<T>::getHandleForEventBus, this),
                std::bind(&ListenForStartOfNextFramePolicy<T>::notificationFromEventBusOfDeletion, this,
                          std::placeholders::_1)});
    }

  private:
    Handle m_listenerHandle;
    T &m_me;
};
} // namespace star::policy