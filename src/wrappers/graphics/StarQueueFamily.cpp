#include "StarQueueFamily.hpp"

star::StarQueueFamily::~StarQueueFamily(){
    if (this->vulkanDevice != nullptr){
        this->vulkanDevice->destroyCommandPool(this->commandPool); 
    }
}

vk::DeviceQueueCreateInfo star::StarQueueFamily::getDeviceCreateInfo(){
    vk::DeviceQueueCreateInfo createInfo{}; 
    createInfo.sType = vk::StructureType::eDeviceQueueCreateInfo; 
    createInfo.queueFamilyIndex = this->queueFamilyIndex;
    createInfo.queueCount = this->queueCount;
    createInfo.pQueuePriorities = this->queuePriority.data();
    return createInfo;
}

void star::StarQueueFamily::init(vk::Device& vulkanDeviceToUse){
    assert(this->vulkanDevice == nullptr && "Init should only be called once");

    this->vulkanDevice = &vulkanDeviceToUse;

    this->commandPool = createCommandPool(this->vulkanDevice, this->queueFamilyIndex);
    this->queues = createQueues(this->vulkanDevice, this->queueFamilyIndex, this->queueCount);
}

vk::Queue& star::StarQueueFamily::getQueue(const uint32_t& index){
    assert(this->vulkanDevice != nullptr && "Must be initialized first"); 
    assert(index < this->queues.size() && "Beyond range of queues");

    return this->queues[index];
}

vk::CommandPool& star::StarQueueFamily::getCommandPool(){
    assert(this->vulkanDevice != nullptr && "Must be initialized first");

    return this->commandPool;
}

const bool star::StarQueueFamily::doesSupport(const star::Queue_Type& type){
    if (star::Queue_Type::Tpresent == type && this->presentSupport){
        return true;
    }else if (star::Queue_Type::Tgraphics == type && this->support & vk::QueueFlagBits::eGraphics){
        return true;
    }else if (star::Queue_Type::Ttransfer == type && this->support & vk::QueueFlagBits::eTransfer){
        return true;
    }else if (star::Queue_Type::Tcompute == type && this->support & vk::QueueFlagBits::eCompute){
        return true;
    }

    return false;
}

vk::CommandPool star::StarQueueFamily::createCommandPool(vk::Device* device, const uint32_t& familyIndex){
    assert(device != nullptr && "Device should not be null");

	vk::CommandPoolCreateInfo commandPoolInfo{};
	commandPoolInfo.sType = vk::StructureType::eCommandPoolCreateInfo;
	commandPoolInfo.queueFamilyIndex = familyIndex;
	commandPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

	vk::CommandPool pool = device->createCommandPool(commandPoolInfo);

	if (!pool) {
		throw std::runtime_error("unable to create pool");
	}

    return pool;
}

std::vector<vk::Queue> star::StarQueueFamily::createQueues(vk::Device* device, const uint32_t& familyIndex, const uint32_t& numToCreate){
    std::vector<vk::Queue> newQueues = std::vector<vk::Queue>(numToCreate);

    for (int i = 0; i < numToCreate; i++){
        newQueues[i] = device->getQueue(familyIndex, i);
    }

    return newQueues;
}