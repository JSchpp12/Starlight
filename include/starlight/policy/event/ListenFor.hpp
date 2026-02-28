#pragma once

#include <star_common/EventBus.hpp>
#include <star_common/Handle.hpp>
#include <star_common/HandleTypeRegistry.hpp>

namespace star::policy::event
{
template <typename TOwner, typename TEvent, auto TTypeNameProvider, auto THandler> class ListenFor
{
  public:
    explicit ListenFor(TOwner &me) : m_registration(), m_me(me)
    {
    }
    ListenFor(const ListenFor<TOwner, TEvent, TTypeNameProvider, THandler> &) = delete;
    ListenFor &operator=(const ListenFor<TOwner, TEvent, TTypeNameProvider, THandler> &) = delete;
    ListenFor(ListenFor<TOwner, TEvent, TTypeNameProvider, THandler> &&) = delete;
    ListenFor &operator=(ListenFor<TOwner, TEvent, TTypeNameProvider, THandler> &&) = delete;
    ~ListenFor() = default;

    void init(star::common::EventBus &eBus)
    {
        registerListener(eBus);
    }

    void cleanup(star::common::EventBus &eBus)
    {
        if (m_registration.isInitialized())
        {
            eBus.unsubscribe(m_registration);
        }
    }

  private:
    star::Handle m_registration;
    TOwner &m_me;

    void registerListener(common::EventBus &eBus)
    {
        auto type = common::HandleTypeRegistry::instance().registerType(TTypeNameProvider());

        eBus.subscribe(
            type,
            star::common::SubscriberCallbackInfo{
                [this](const common::IEvent &e, bool &keepAlive) {
                    const auto &event = static_cast<const TEvent &>(e);
                    std::invoke(THandler, this->m_me, event, keepAlive);
                },
                std::bind(&ListenFor<TOwner, TEvent, TTypeNameProvider, THandler>::getHandleForEventBus, this),
                std::bind(&ListenFor<TOwner, TEvent, TTypeNameProvider, THandler>::notificationFromEventbusOfDeletion,
                          this, std::placeholders::_1)});
    }

    Handle *getHandleForEventBus()
    {
        return &m_registration;
    }

    void notificationFromEventbusOfDeletion(const Handle &noLongerNeededHandle)
    {
        if (m_registration == noLongerNeededHandle)
        {
            m_registration = Handle();
        }
    }
};
} // namespace star::policy::event