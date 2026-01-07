#pragma once

#include "starlight/event/RenderReadyForFinalization.hpp"

#include <star_common/EventBus.hpp>
#include <star_common/Handle.hpp>

#include <concepts>

namespace star::policy
{
template <typename T>
concept ListenerRenderReadyLike =
    requires(T listener, const event::RenderReadyForFinalization &event, bool &keepAlive) {
        { listener.onRenderReadyForFinalization(event, keepAlive) } -> std::same_as<void>;
    };

template <typename T> class ListenForRenderReadyForFinalization
{
  public:
    explicit ListenForRenderReadyForFinalization(T &me) : m_listenerHandle(), m_me(me)
    {
    }
    ListenForRenderReadyForFinalization(const ListenForRenderReadyForFinalization &) = delete;
    ListenForRenderReadyForFinalization &operator=(const ListenForRenderReadyForFinalization &) = delete;
    ListenForRenderReadyForFinalization(ListenForRenderReadyForFinalization &&) = delete;
    ListenForRenderReadyForFinalization &operator=(ListenForRenderReadyForFinalization &&) = delete;
    ~ListenForRenderReadyForFinalization() = default;

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
            common::HandleTypeRegistry::instance().registerType(event::GetRenderReadyForFinalizationTypeName),
            common::SubscriberCallbackInfo{
                std::bind(&ListenForRenderReadyForFinalization<T>::eventCallback, this, std::placeholders::_1,
                          std::placeholders::_2),
                std::bind(&ListenForRenderReadyForFinalization<T>::getHandleForEventBus, this),
                std::bind(&ListenForRenderReadyForFinalization<T>::notificationFromEventBusOfDelet, this,
                          std::placeholders::_1)});
    }

    void eventCallback(const common::IEvent &e, bool &keepAlive)
        requires ListenerRenderReadyLike<T>
    {
        const auto &event = static_cast<const event::RenderReadyForFinalization &>(e);

        m_me.onRenderReadyForFinalization(event, keepAlive);
    }

    void notificationFromEventBusOfDelet(const Handle &noLongerNeeded)
    {
        if (noLongerNeeded == m_listenerHandle)
        {
            m_listenerHandle = Handle();
        }
    }
};
} // namespace star::policy