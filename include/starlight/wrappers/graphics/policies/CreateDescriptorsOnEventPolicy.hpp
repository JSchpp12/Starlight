#pragma once

#include "core/device/StarDevice.hpp"
#include "core/device/managers/DescriptorPool.hpp"
#include <star_common/EventBus.hpp>
#include <star_common/HandleTypeRegistry.hpp>

#include <concepts>
#include <memory>

namespace star::wrappers::graphics::policies
{
template <typename TCreateDescriptorsPolicy>
concept CreateDescriptorsPolicyLike = requires(TCreateDescriptorsPolicy p) {
    { p.create() } -> std::same_as<void>;
};

template <CreateDescriptorsPolicyLike TCreateDescriptorsPolicy>
class CreateDescriptorsOnEventPolicy
    : public std::enable_shared_from_this<CreateDescriptorsOnEventPolicy<TCreateDescriptorsPolicy>>
{
  public:
    class Builder
    {
      public:
        Builder(common::EventBus &bus) : m_bus(bus)
        {
        }
        Builder &setEventType(const uint16_t &type)
        {
            m_type = type;
            return *this;
        }
        Builder &setPolicy(TCreateDescriptorsPolicy policy)
        {
            m_policy = std::move(policy);
            return *this;
        }
        std::shared_ptr<CreateDescriptorsOnEventPolicy<TCreateDescriptorsPolicy>> buildShared()
        {
            auto result = std::shared_ptr<CreateDescriptorsOnEventPolicy<TCreateDescriptorsPolicy>>(
                new CreateDescriptorsOnEventPolicy<TCreateDescriptorsPolicy>(std::move(m_policy)));

            result->init(m_type, m_bus);

            return result;
        }

      private:
        common::EventBus &m_bus;
        uint16_t m_type;
        TCreateDescriptorsPolicy m_policy;
    };

  private:
    TCreateDescriptorsPolicy m_create;
    Handle m_registeredCallback;

    CreateDescriptorsOnEventPolicy(TCreateDescriptorsPolicy create) : m_create(std::move(create))
    {
    }

    void init(const uint16_t &eventType, common::EventBus &eventBus)
    {
        registerWithEventBus(eventType, eventBus);
    }

    void noLongerNeededHandle(const Handle &handle)
    {
        if (m_registeredCallback == handle)
        {
            m_registeredCallback = Handle();
        }
    }

    void eventCallback(const common::IEvent &e, bool &keepAlive)
    {
        (void)e;
        
        m_create.create();
        keepAlive = false;
    }

    void registerWithEventBus(const uint16_t &eventType, common::EventBus &eventBus)
    {
        auto self = this->shared_from_this();

        eventBus.subscribe(
            eventType,
            common::SubscriberCallbackInfo{
                [self](const common::IEvent &e, bool &keepAlive) { self->eventCallback(e, keepAlive); },
                [self]() { return &self->m_registeredCallback; },
                [self](const Handle &noLongerNeededHandle) { self->noLongerNeededHandle(noLongerNeededHandle); }});
    }

    friend class Builder;
};
} // namespace star::wrappers::graphics::policies