#pragma once

#include "starlight/event/RegisterMainGraphicsRenderer.hpp"
#include <star_common/Handle.hpp>
#include <star_common/HandleTypeRegistry.hpp>

#include <concepts>

namespace star::policy
{
template <typename T>
concept OnRegisterMainGraphicsListenerLike =
    requires(T listener, const star::event::RegisterMainGraphicsRenderer &event, bool &keepAlive) {
        { listener.onRegisterMainGraphics(event, keepAlive) } -> std::same_as<void>;
    };

template <typename T> class ListenForRegisterMainGraphicsRenderPolicy
{
  public:
    explicit ListenForRegisterMainGraphicsRenderPolicy(T &me)
        requires OnRegisterMainGraphicsListenerLike<T>
        : m_listenerHandle(), m_me(me){};

    ListenForRegisterMainGraphicsRenderPolicy(const ListenForRegisterMainGraphicsRenderPolicy &) = delete;
    ListenForRegisterMainGraphicsRenderPolicy &operator=(const ListenForRegisterMainGraphicsRenderPolicy &) = delete;
    ListenForRegisterMainGraphicsRenderPolicy(ListenForRegisterMainGraphicsRenderPolicy &&) = delete;
    ListenForRegisterMainGraphicsRenderPolicy &operator=(ListenForRegisterMainGraphicsRenderPolicy &&) = delete;
    ~ListenForRegisterMainGraphicsRenderPolicy() = default;

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
            common::HandleTypeRegistry::instance().registerType(star::event::GetRegisterMainGraphicsRendererTypeName()),
            common::SubscriberCallbackInfo{
                std::bind(&ListenForRegisterMainGraphicsRenderPolicy<T>::eventCallback, this, std::placeholders::_1,
                          std::placeholders::_2),
                std::bind(&ListenForRegisterMainGraphicsRenderPolicy<T>::getHandleForEventBus, this),
                std::bind(&ListenForRegisterMainGraphicsRenderPolicy<T>::notificationFromEventBusOfDeletion, this,
                          std::placeholders::_1)});
    }

    void eventCallback(const common::IEvent &e, bool &keepAlive)
    {
        const auto &event = static_cast<const star::event::RegisterMainGraphicsRenderer &>(e);

        m_me.onRegisterMainGraphics(event, keepAlive);
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