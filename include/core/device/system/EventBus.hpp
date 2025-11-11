#pragma once

#include "CastHelpers.hpp"
#include "Handle.hpp"
#include "SubscriberCallbackInfo.hpp"

#include <starlight/common/IEvent.hpp>

#include <cassert>
#include <functional>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace star::core::device::system
{
class EventBus
{
  public:
    template <typename TEventType>
    void subscribe(SubscriberCallbackInfo callbackInfo)
    {
        const size_t key = getKey<TEventType>();

        m_listeners[key].push_back(callbackInfo);

        uint32_t id;
        CastHelpers::SafeCast<size_t, uint32_t>(m_listeners[key].size(), id);
        id--;

        auto *handle = callbackInfo.doCallbackGetSubscriberHandleForUpdate();
        *handle = Handle{.type = Handle_Type::subscriber, .id = id};
    }

    template <typename TEventType> void emit(const TEventType &event)
    {
        const size_t key = getKey<TEventType>();
        const auto &base = static_cast<const star::common::IEvent &>(event);

        std::vector<Callback> aliveCallbacks = std::vector<Callback>();

        if (!m_listeners.contains(key))
        {
            throw std::runtime_error("Provided event type does not have a registered handler");
        }
        auto indicesToRemove = std::vector<size_t>();


        for (size_t i = 0; i < m_listeners[key].size(); i++)
        {
            bool keepAlive = false;
            m_listeners[key][i].doCallback(base, keepAlive);

            if (!keepAlive)
            {
                indicesToRemove.push_back(i);
            }
        }

        if (indicesToRemove.size() > 0)
        {
            for (int i = indicesToRemove.size() - 1; i >= 0; i--)
            {
                const Handle *subHandle = m_listeners[key][indicesToRemove[i]].doCallbackGetSubscriberHandleForUpdate();
                if (subHandle != nullptr)
                {
                    removeSubscriber(*subHandle, key, m_listeners[key]);
                }
            }
        }
    }

    template <typename TEventType> void unsubscribe(const Handle &subscriberHandle)
    {
        const size_t key = getKey<TEventType>();

        assert(m_listeners.contains(key));

        removeSubscriber(subscriberHandle, key, m_listeners[key]);
    }

  private:
    std::unordered_map<size_t, std::vector<SubscriberCallbackInfo>> m_listeners;

    template <typename TEventType> size_t getKey()
    {
        static_assert(std::is_base_of_v<star::common::IEvent, TEventType>, "TEventType must derive from Event");
        return typeid(TEventType).hash_code();
    }

    void removeSubscriber(const Handle &subscriberHandle, const size_t &key,
                          std::vector<SubscriberCallbackInfo> &subs)
    {
        size_t handleID = 0;
        CastHelpers::SafeCast<uint32_t, size_t>(subscriberHandle.getID(), handleID);

        assert(handleID < subs.size() && "Handle does not correlate to any listener registered for the template type");

        subs[handleID].doCallbackNotifySubscriberHandleCanBeDeleted(subscriberHandle);

        // Shift elements left from handleID+1 to end
        for (size_t i = handleID + 1; i < subs.size(); ++i)
        {
            subs[i - 1] = std::move(subs[i]);

            // Update the moved subscriber's handle (its index decreased by 1)
            if (Handle *h = subs[i - 1].doCallbackGetSubscriberHandleForUpdate())
            {
                if (h->id > 0)
                {
                    h->id -= 1;
                }
            }
        }

        // Actually remove the trailing duplicate after the shift
        subs.pop_back();
    }
};
} // namespace star::core::device::system