#pragma once

#include "CastHelpers.hpp"
#include "Handle.hpp"
#include "device/system/event/EventBase.hpp"

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
    using Callback = std::function<void(const EventBase &, bool &)>;
    using CallbackGetSubscriberHandleForUpdate = std::function<Handle *(void)>;
    using CallbackNotifySubscriberHandleCanBeDeleted = std::function<void(const Handle &)>;

  public:
    template <typename TEventType>
    void subscribe(Callback callback, CallbackGetSubscriberHandleForUpdate getHandleCallback,
                   CallbackNotifySubscriberHandleCanBeDeleted notifySubscriberOfDeleteHandle)
    {
        const size_t key = getKey<TEventType>();

        m_listeners[key].push_back(
            std::make_pair(callback, HandleUpdateInfo{.getHandleForUpdateCallback = getHandleCallback,
                                                      .deleteHandleCallback = notifySubscriberOfDeleteHandle}));

        uint32_t id;
        CastHelpers::SafeCast<size_t, uint32_t>(m_listeners[key].size(), id);
        id--;

        auto *handle = getHandleCallback();
        *handle = Handle{.type = Handle_Type::subscriber, .id = id};
    }

    template <typename TEventType> void emit(const TEventType &event)
    {
        const size_t key = getKey<TEventType>();
        const auto &base = static_cast<const EventBase &>(event);

        std::vector<Callback> aliveCallbacks = std::vector<Callback>();

        if (!m_listeners.contains(key))
        {
            throw std::runtime_error("Provided event type does not have a registered handler");
        }
        auto indicesToRemove = std::vector<size_t>();

        for (size_t i = 0; i < m_listeners[key].size(); i++)
        {
            bool keepAlive = false;
            m_listeners[key][i].first(base, keepAlive);

            if (!keepAlive)
            {
                indicesToRemove.push_back(i);
            }
        }

        if (indicesToRemove.size() > 0)
        {
            for (int i = indicesToRemove.size() - 1; i >= 0; i--)
            {
                const Handle *subHandle = m_listeners[key][indicesToRemove[i]].second.getHandleForUpdateCallback();
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
    struct HandleUpdateInfo
    {
        CallbackGetSubscriberHandleForUpdate getHandleForUpdateCallback;
        CallbackNotifySubscriberHandleCanBeDeleted deleteHandleCallback;
    };
    std::unordered_map<size_t, std::vector<std::pair<Callback, HandleUpdateInfo>>> m_listeners;

    template <typename TEventType> size_t getKey()
    {
        static_assert(std::is_base_of_v<EventBase, TEventType>, "TEventType must derive from Event");
        return typeid(TEventType).hash_code();
    }

    void removeSubscriber(const Handle &subscriberHandle, const size_t &key,
                          std::vector<std::pair<Callback, HandleUpdateInfo>> &subs)
    {
        // Validate the handle index
        size_t handleID = 0;
        CastHelpers::SafeCast<uint32_t, size_t>(subscriberHandle.getID(), handleID);
        assert(handleID < subs.size() && "Handle does not correlate to any listener registered for the template type");

        // Notify the removed subscriber that its handle can be deleted
        {
            uint32_t removedId32 = 0;
            CastHelpers::SafeCast<size_t, uint32_t>(handleID, removedId32);
            subs[handleID].second.deleteHandleCallback(Handle{.type = Handle_Type::subscriber, .id = removedId32});
        }

        // Shift elements left from handleID+1 to end
        for (size_t i = handleID + 1; i < subs.size(); ++i)
        {
            subs[i - 1] = std::move(subs[i]); // move to avoid copying std::function

            // Update the moved subscriber's handle (its index decreased by 1)
            if (Handle *h = subs[i - 1].second.getHandleForUpdateCallback())
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