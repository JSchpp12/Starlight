#pragma once

#include "Enums.hpp"
#include "StarQueue.hpp"
#include "StarCommandPool.hpp"

#include <vulkan/vulkan.hpp>

#include <vector>
#include <memory>

namespace star
{
class StarQueueFamily
{
  public:
    ~StarQueueFamily();
    StarQueueFamily(const uint32_t &queueFamilyIndex, const uint32_t &queueCount, const vk::QueueFlags &support,
                    const bool &presentationSupport);

    vk::DeviceQueueCreateInfo getDeviceCreateInfo();

    void init(vk::Device &vulkanDeviceToUse);

    std::vector<StarQueue> &getQueues(){
        return this->queues;
    }

    std::shared_ptr<StarCommandPool> createCommandPool(const bool &setAutoReset);

    bool doesSupport(const vk::QueueFlags &querySupport, const bool &queryPresentationSupport) const;

    bool doesSupport(const vk::QueueFlags &querySupport) const;

    uint32_t getQueueCount() const
    {
        return this->queueCount;
    }

    uint32_t getQueueFamilyIndex() const
    {
        return this->queueFamilyIndex;
    }

  private:
    uint32_t queueFamilyIndex;
    uint32_t queueCount;
    vk::QueueFlags support;
    bool presentationSupport;
    std::vector<float> queuePriority = std::vector<float>();

    vk::Device *vulkanDevice = nullptr;

    std::vector<StarQueue> queues;

    static std::vector<StarQueue> CreateQueues(vk::Device *device, const uint32_t &familyIndex,
                                               const uint32_t &numToCreate);
};
} // namespace star