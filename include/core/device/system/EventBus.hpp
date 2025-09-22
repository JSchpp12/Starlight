#pragma once

#include "device/system/event/EventBase.hpp"

#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace star::core::device::system
{

class EventBus
{
    using Callback = std::function<void(const EventBase &, bool &)>;

  public:
    template <typename TEventType> void subscribe(Callback callback)
    {
        const size_t key = getKey<TEventType>(); 

        m_listeners[key].push_back(callback); 
    }

    template<typename TEventType> void emit(const TEventType &event)
    {
        const size_t key = getKey<TEventType>(); 
        const auto& base = static_cast<const EventBase &>(event); 

        std::vector<Callback> aliveCallbacks = std::vector<Callback>(); 

        for (auto &cb : m_listeners[key])
        {
            bool keepAlive = false; 
            cb(base, keepAlive); 

            if (keepAlive){
                aliveCallbacks.push_back(cb); 
            }
        }

        m_listeners[key] = aliveCallbacks; 
    }

  private:
    std::unordered_map<size_t, std::vector<std::function<void(const EventBase &, bool &)>>> m_listeners;

    template<typename TEventType>
    size_t getKey(){
        static_assert(std::is_base_of_v<EventBase, TEventType>, "TEventType must derive from Event");
        return typeid(TEventType).hash_code();
    }
};
} // namespace star::core::device::system