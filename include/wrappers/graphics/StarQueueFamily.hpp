#pragma once

#include "Enums.hpp"

#include <vulkan/vulkan.hpp>

#include <vector>

namespace star{
    class StarQueueFamily{
        public:
        const float priority = 1.0f;

        ~StarQueueFamily(); 
        StarQueueFamily(const uint32_t& queueFamilyIndex, const uint32_t& queueCount, const vk::QueueFlags& support, const bool& supportsPresentation) 
        : queueFamilyIndex(queueFamilyIndex), queueCount(queueCount), support(support), presentSupport(presentSupport){
            this->queuePriority.push_back(1.0f);
            for (int i = 1; i < queueCount; i++){
                this->queuePriority.push_back(0.0f);
            }
        }; 

        vk::DeviceQueueCreateInfo getDeviceCreateInfo(); 

        void init(vk::Device& vulkanDeviceToUse); 

        vk::Queue& getQueue(const uint32_t& index = 0);

        vk::CommandPool& getCommandPool();

        const bool doesSupport(const star::Queue_Type& type); 

        const uint32_t& getQueueCount(){
            return this->queueCount;
        }

        private:
        const uint32_t queueFamilyIndex;  
        const uint32_t queueCount; 
        const vk::QueueFlags support;
        const bool presentSupport; 
        std::vector<float> queuePriority = std::vector<float>(); 

        vk::Device* vulkanDevice = nullptr;

        std::vector<vk::Queue> queues;
        vk::CommandPool commandPool; 

        static vk::CommandPool createCommandPool(vk::Device* device, const uint32_t& familyIndex); 

        static std::vector<vk::Queue> createQueues(vk::Device* device, const uint32_t& familyIndex, const uint32_t& numToCreate);
    };
}