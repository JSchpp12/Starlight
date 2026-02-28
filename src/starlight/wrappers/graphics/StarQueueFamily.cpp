#include "StarQueueFamily.hpp"

star::StarQueueFamily::StarQueueFamily(const uint32_t &queueFamilyIndex, const uint32_t &queueCount,
                                       const vk::QueueFlags &support, const bool &presentationSupport)
    : queueFamilyIndex(queueFamilyIndex), queueCount(queueCount), support(support),
      presentationSupport(presentationSupport)
{
    for (uint32_t i{0}; i < queueCount; i++)
    {
        this->queuePriority.push_back(1.0f);
    }
};

void star::StarQueueFamily::init(vk::Device &vulkanDeviceToUse)
{
    assert(this->vulkanDevice == nullptr && "Init should only be called once");

    this->vulkanDevice = &vulkanDeviceToUse;

    this->queues = createQueues(this->vulkanDevice, this->queueFamilyIndex, static_cast<size_t>(this->queueCount));
}

std::shared_ptr<star::StarCommandPool> star::StarQueueFamily::createCommandPool(const bool &setAutoReset)
{
    assert(this->vulkanDevice != nullptr && "Must be initialized first");

    return std::make_shared<StarCommandPool>(*this->vulkanDevice, this->queueFamilyIndex, setAutoReset);
}

bool star::StarQueueFamily::doesSupport(const vk::QueueFlags &querySupport, const bool &querySupportPresentation) const
{
    if (!querySupportPresentation ||
        (querySupportPresentation && this->presentationSupport == querySupportPresentation))
    {
        return doesSupport(querySupport);
    }

    return false;
}

bool star::StarQueueFamily::doesSupport(const vk::QueueFlags &querySupport) const
{
    return (this->support & querySupport) == querySupport;
}

std::vector<star::StarQueue> star::StarQueueFamily::createQueues(vk::Device *device, const uint32_t &familyIndex,
                                                                 const size_t &numToCreate)
{
    std::vector<vk::Queue> newQueues = std::vector<vk::Queue>(numToCreate);
    std::vector<StarQueue> finalQueues = std::vector<StarQueue>(numToCreate);

    for (size_t i{0}; i < numToCreate; i++)
    {
        newQueues[i] = device->getQueue(familyIndex, i);
        finalQueues[i] = StarQueue(familyIndex, newQueues[i], support, presentationSupport);
    }

    return finalQueues;
}