#pragma once

#include "starlight/wrappers/graphics/StarQueueFamily.hpp"

#include <set>
#include <vulkan/vulkan.hpp>
#include <unordered_map>

namespace star
{

class QueueFamilyIndices
{
  public:
    void registerFamily(const uint32_t &familyIndex, const vk::QueueFlags &queueSupport,
                        const vk::Bool32 &presentSupport, const uint32_t &familyQueueCount);

    // check if queue families are all separate -- this means more parallel work
    bool isOptimalSupport(const bool needsPresentationSupport) const;

    bool isFullySupported(const bool needsPresentationSupport) const;

    bool isSuitable(const bool needsPresentationSupport) const;

    uint32_t getNumQueuesForIndex(const uint32_t &index)
    {
        return this->familyIndexQueueCount[index];
    }

    vk::QueueFlags getSupportForIndex(const uint32_t &index)
    {
        return this->familyIndexQueueSupport[index];
    }

    const std::set<uint32_t> &getUniques() const
    {
        return this->allIndicies;
    }

    bool getSupportsPresentForIndex(const uint32_t &index)
    {
        return this->presentFamilies.find(index) != this->presentFamilies.end();
    }

    std::vector<StarQueueFamily> getQueueFamilies(); 

  private:
    std::set<uint32_t> allIndicies = std::set<uint32_t>();
    std::unordered_map<uint32_t, vk::QueueFlags> familyIndexQueueSupport =
        std::unordered_map<uint32_t, vk::QueueFlags>();
    std::unordered_map<uint32_t, uint32_t> familyIndexQueueCount = std::unordered_map<uint32_t, uint32_t>();
    std::set<uint32_t> graphicsFamilies = std::set<uint32_t>();
    std::set<uint32_t> presentFamilies = std::set<uint32_t>();
    std::set<uint32_t> transferFamilies = std::set<uint32_t>();
    std::set<uint32_t> computeFamilies = std::set<uint32_t>();
};
} // namespace star