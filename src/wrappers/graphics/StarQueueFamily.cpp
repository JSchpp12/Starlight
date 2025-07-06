#include "StarQueueFamily.hpp"

star::StarQueueFamily::~StarQueueFamily()
{
}

star::StarQueueFamily::StarQueueFamily(const uint32_t &queueFamilyIndex, const uint32_t &queueCount,
                                       const vk::QueueFlags &support, const bool &presentationSupport)
    : queueFamilyIndex(queueFamilyIndex), queueCount(queueCount), support(support), presentationSupport(presentationSupport)
{
    this->queuePriority.push_back(1.0f);
    for (int i = 1; i < queueCount; i++)
    {
        this->queuePriority.push_back(0.0f);
    }
};

vk::DeviceQueueCreateInfo star::StarQueueFamily::getDeviceCreateInfo()
{
    vk::DeviceQueueCreateInfo createInfo{};
    createInfo.sType = vk::StructureType::eDeviceQueueCreateInfo;
    createInfo.queueFamilyIndex = this->queueFamilyIndex;
    createInfo.queueCount = this->queueCount;
    createInfo.pQueuePriorities = this->queuePriority.data();
    return createInfo;
}

void star::StarQueueFamily::init(vk::Device &vulkanDeviceToUse)
{
    assert(this->vulkanDevice == nullptr && "Init should only be called once");

    this->vulkanDevice = &vulkanDeviceToUse;

    this->queues = CreateQueues(this->vulkanDevice, this->queueFamilyIndex, this->queueCount);
}

std::shared_ptr<star::StarCommandPool> star::StarQueueFamily::createCommandPool(const bool &setAutoReset)
{
    assert(this->vulkanDevice != nullptr && "Must be initialized first");

    return std::make_shared<StarCommandPool>(*this->vulkanDevice, this->queueFamilyIndex, setAutoReset);
}

bool star::StarQueueFamily::doesSupport(const vk::QueueFlags &querySupport, const bool &querySupportPresentation) const{
    if (!querySupportPresentation || (querySupportPresentation && this->presentationSupport == querySupportPresentation)){
        return doesSupport(querySupport); 
    }

    return false;
}

bool star::StarQueueFamily::doesSupport(const vk::QueueFlags &querySupport) const
{
    return (this->support & querySupport) == querySupport;
}

std::vector<star::StarQueue> star::StarQueueFamily::CreateQueues(vk::Device *device, const uint32_t &familyIndex,
                                                           const uint32_t &numToCreate)
{
    std::vector<vk::Queue> newQueues = std::vector<vk::Queue>(numToCreate);

    for (int i = 0; i < numToCreate; i++)
    {
        newQueues[i] = device->getQueue(familyIndex, i);
    }

    std::vector<StarQueue> finalQueues = std::vector<StarQueue>(numToCreate);
    for (int i = 0; i < numToCreate; i++){
        finalQueues[i] = StarQueue(newQueues[i], familyIndex); 
    }

    return finalQueues;
}