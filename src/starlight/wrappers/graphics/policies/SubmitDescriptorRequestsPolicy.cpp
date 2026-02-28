#include "wrappers/graphics/policies/SubmitDescriptorRequestsPolicy.hpp"

#include "event/ConsumeDescriptorRequests.hpp"

#include <star_common/HandleTypeRegistry.hpp>

namespace star::wrappers::graphics::policies
{

void SubmitDescriptorRequestsPolicy::init(common::EventBus &eventBus)
{
    subscribeToEventBus(eventBus);
}

void SubmitDescriptorRequestsPolicy::subscribeToEventBus(common::EventBus &eventBus)
{
    auto sharedThis = shared_from_this();

    if (!common::HandleTypeRegistry::instance().contains(star::event::GetConsumeDescriptorRequestsTypeName))
    {
        common::HandleTypeRegistry::instance().registerType(star::event::GetConsumeDescriptorRequestsTypeName);
    }

    eventBus.subscribe(
        common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
            star::event::GetConsumeDescriptorRequestsTypeName),
        common::SubscriberCallbackInfo{
            [sharedThis](const star::common::IEvent &e, bool &keepAlive) { sharedThis->eventCallback(e, keepAlive); },
            [sharedThis]() { return sharedThis->getHandleForUpdate(); },
            [sharedThis](const Handle &handle) { sharedThis->notificationFromEventBusHandleDelete(handle); }});
}

void SubmitDescriptorRequestsPolicy::notificationFromEventBusHandleDelete(const Handle &noLongerNeededSubscriberHandle)
{
    if (m_subscriberHandle == noLongerNeededSubscriberHandle)
    {
        m_subscriberHandle = Handle();
    }
}

Handle *SubmitDescriptorRequestsPolicy::getHandleForUpdate()
{
    return &m_subscriberHandle;
}

void SubmitDescriptorRequestsPolicy::eventCallback(const common::IEvent &e, bool &keepAlive)
{
    const auto &event = static_cast<const event::ConsumeDescriptorRequests &>(e);
    std::vector<std::pair<vk::DescriptorType, const uint32_t>> *data = event::GetDestination(event);

    for (auto &request : m_descriptorRequests)
    {
        data->push_back(request);
    }

    keepAlive = false;
}

} // namespace star::wrappers::graphics::policies
