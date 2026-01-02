#pragma once

#include "starlight/event/EnginePhaseComplete.hpp"

#include <star_common/EventBus.hpp>
#include <star_common/Handle.hpp>

#include <concepts>

namespace star::policy
{
template <typename T>
concept ListenerEnginePhaseLike = requires(T listener, const event::EnginePhaseComplete &event, bool &keepAlive) {
    { listener.onEnginePhaseComplete(event, keepAlive) } -> std::same_as<void>;
};

template <typename T> class ListenForEnginePhaseCompletePolicy
{
  public:
    ListenForEnginePhaseCompletePolicy(T &me)
        requires ListenerEnginePhaseLike<T>
        : m_listenerHandle(), m_me(me)
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
            common::HandleTypeRegistry::instance().registerType(star::event::GetEnginePhaseCompleteLoadTypeName),
            common::SubscriberCallbackInfo{
                std::bind(&ListenForEnginePhaseCompletePolicy<T>::eventCallback, this, std::placeholders::_1,
                          std::placeholders::_2, std::placeholders::_3),
                std::bind(&ListenForEnginePhaseCompletePolicy<T>::getHandleForEventBus, this),
                std::bind(&ListenForEnginePhaseCompletePolicy<T>::notificationOfEventBusOfDeletion, this,
                          std::placeholders::_1)});
    }

    void eventCallback(const common::IEvent &e, bool &keepAlive)
    {
        const auto &event = static_cast<const event::EnginePhaseComplete &>(e);
        m_me.onEnginePhaseComplete(event, keepAlive);
    }

    void notificationFromEventBusOfDeletion(const Handle &noLongerNeededHandle)
    {
        if (noLongerNeededHandle == m_listenerHandle)
        {
            m_listenerHandle = Handle();
        }
    }
};
} // namespace star::policy