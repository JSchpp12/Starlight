#pragma once

#include <star_common/EventBus.hpp>
#include <star_common/Handle.hpp>
#include <star_common/HandleTypeRegistry.hpp>
#include <starlight/event/PrepForNextFrame.hpp>

namespace star::policy
{
template <typename T>
concept ListenerForPrepForNextFrameLike = requires(T listener, const event::PrepForNextFrame &event, bool &keepAlive) {
    { listener.onPrepForNextFrame(event, keepAlive) } -> std::same_as<void>;
};

template <typename T> class ListenForPrepForNextFramePolicy
{
  public:
    explicit ListenForPrepForNextFramePolicy(T &me) : m_listenerHandle(), m_me(me)
    {
    }
    ListenForPrepForNextFramePolicy(const ListenForPrepForNextFramePolicy &) = delete;
    ListenForPrepForNextFramePolicy &operator=(const ListenForPrepForNextFramePolicy &) = delete;
    ListenForPrepForNextFramePolicy(ListenForPrepForNextFramePolicy &&) = delete;
    ListenForPrepForNextFramePolicy &operator=(ListenForPrepForNextFramePolicy &&) = delete;
    ~ListenForPrepForNextFramePolicy() = default;

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

  protected:
    Handle m_listenerHandle;
    T &m_me;

    void registerListener(common::EventBus &eventBus)
    {
        eventBus.subscribe(common::HandleTypeRegistry::instance().registerType(event::GetPrepForNextFrameEventTypeName),
                           common::SubscriberCallbackInfo{
                               std::bind(&ListenForPrepForNextFramePolicy<T>::eventCallback, this,
                                         std::placeholders::_1, std::placeholders::_2),
                               std::bind(&ListenForPrepForNextFramePolicy<T>::getHandleForEventBus, this),
                               std::bind(&ListenForPrepForNextFramePolicy<T>::notificationFromEventBusOfDeletion, this,
                                         std::placeholders::_1)});
    }

    void eventCallback(const common::IEvent &e, bool &keepAlive)
    {
        const auto &event = static_cast<const event::PrepForNextFrame &>(e);

        m_me.onPrepForNextFrame(event, keepAlive);
    }

    Handle *getHandleForEventBus()
    {
        return &m_listenerHandle;
    }

    void notificationFromEventBusOfDeletion(const Handle &noLongerNeededHandle)
    {
        if (m_listenerHandle == noLongerNeededHandle)
        {
            m_listenerHandle = Handle{};
        }
    }
};
} // namespace star::policy