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

        for (auto &cb : m_listeners[key])
        {
            bool keepAlive = false;
            cb.first(base, keepAlive);

            // if (keepAlive)
            // {
            //     aliveCallbacks.push_back(cb);
            // }
        }

        // m_listeners[key] = aliveCallbacks;
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
                          std::vector<std::pair<Callback, HandleUpdateInfo>> &resultingSubscribers)
    {

        assert(subscriberHandle.getID() < m_listeners[key].size() &&
               "Hande does not corelate to any listener registered for the template type");
        assert(m_listeners.contains(key) && "No listeners are registered for the template type");

        size_t handleID;
        CastHelpers::SafeCast<uint32_t, size_t>(subscriberHandle.getID(), handleID);

        auto currentSubscribers = std::vector<std::pair<Callback, HandleUpdateInfo>>(resultingSubscribers.size() - 1);
        bool isBeyondChanged = false;
        for (size_t i = 0; i < resultingSubscribers.size(); i++)
        {
            if (!isBeyondChanged)
            {
                if (i == handleID)
                {
                    // this is the one to remove
                    isBeyondChanged = true;
                    uint32_t id = 0;
                    CastHelpers::SafeCast<size_t, uint32_t>(i, id);

                    currentSubscribers[i].second.deleteHandleCallback(
                        Handle{.type = Handle_Type::subscriber, .id = id});
                }
                else
                {
                    currentSubscribers[i] = std::move(currentSubscribers[i]);
                }
            }
            else
            {
                currentSubscribers[i - 1] = currentSubscribers[i];
                auto *handleToUpdate = currentSubscribers[i - 1].second.getHandleForUpdateCallback();
                handleToUpdate->id--;
            }
        }

        resultingSubscribers = std::move(currentSubscribers);
    }

    void updateHandle(const Handle &newHandleValues, CallbackGetSubscriberHandleForUpdate &callback)
    {
        Handle *containerHandle = callback();
        if (containerHandle != nullptr)
        {
            containerHandle->id = newHandleValues.getID();
            containerHandle->type = newHandleValues.getType();
        }
    }
};
} // namespace star::core::device::system