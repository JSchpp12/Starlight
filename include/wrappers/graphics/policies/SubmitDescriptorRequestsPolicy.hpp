#pragma once

#include <starlight/common/EventBus.hpp>
#include <starlight/common/IEvent.hpp>

#include <vulkan/vulkan.hpp>

#include <functional>
#include <memory>
#include <vector>

namespace star::wrappers::graphics::policies
{
class SubmitDescriptorRequestsPolicy : public std::enable_shared_from_this<SubmitDescriptorRequestsPolicy>
{
  public:
    explicit SubmitDescriptorRequestsPolicy(
        std::vector<std::pair<vk::DescriptorType, const uint32_t>> descriptorRequests)
        : m_descriptorRequests(std::move(descriptorRequests))
    {
    }

    void init(common::EventBus &eventBus);

  private:
    Handle m_subscriberHandle;
    std::vector<std::pair<vk::DescriptorType, const uint32_t>> m_descriptorRequests;

    void subscribeToEventBus(common::EventBus &bus);

    void notificationFromEventBusHandleDelete(const Handle &noLongerNeededSubscriberHandle);

    Handle *getHandleForUpdate();

    void eventCallback(const common::IEvent &e, bool &keepAlive);
};
} // namespace star::wrappers::graphics::policies