#include "starlight/service/detail/queue_ownership/QueueOwnershipTracker.hpp"

namespace star::service::detail::queue_ownership
{
QueueOwnershipTracker::QueueOwnershipTracker(std::vector<StarQueueFamily> reqQueueFamilies)
    : regQueueFamilies(regQueueFamilies), isQueueAvailable(std::vector<std::vector<bool>>(regQueueFamilies.size()))
{
    for (size_t i = 0; i < this->regQueueFamilies.size(); i++)
    {
        this->isQueueAvailable.at(i) = std::vector<bool>(this->regQueueFamilies.at(i).getQueueCount(), true);
    }
}
std::optional<StarQueue> QueueOwnershipTracker::giveMeQueueWithProperties(const vk::QueueFlags &capabilities,
                                                                          const bool &presentationSupport,
                                                                          const uint32_t &familyIndex)
{
    for (size_t i = 0; i < this->regQueueFamilies.size(); i++)
    {
        if (this->regQueueFamilies.at(i).getQueueFamilyIndex() == familyIndex &&
            this->regQueueFamilies.at(i).doesSupport(capabilities, presentationSupport))
        {
            for (size_t j = 0; j < this->isQueueAvailable.at(i).size(); j++)
            {
                if (this->isQueueAvailable.at(i).at(j))
                {
                    this->isQueueAvailable.at(i).at(j) = false;
                    return std::make_optional(this->regQueueFamilies.at(i).getQueues().at(j));
                }
            }
        }
    }

    return std::nullopt;
}

std::vector<uint32_t> QueueOwnershipTracker::getQueueFamiliesWhichSupport(const vk::QueueFlags &capabilities,
                                                                          const bool presentationSupport)
{
    std::vector<uint32_t> indices = std::vector<uint32_t>();

    for (size_t i = 0; i < this->regQueueFamilies.size(); i++)
    {
        if (this->regQueueFamilies.at(i).doesSupport(capabilities, presentationSupport))
        {
            indices.push_back(this->regQueueFamilies.at(i).getQueueFamilyIndex());
        }
    }

    return indices;
}

std::vector<uint32_t> QueueOwnershipTracker::getAllQueueFamilyIndices() const
{
    std::vector<uint32_t> indices = std::vector<uint32_t>(regQueueFamilies.size());
    for (size_t i{0}; this->regQueueFamilies.size(); i++)
    {
        indices[i] = this->regQueueFamilies.at(i).getQueueFamilyIndex();
    }

    return indices;
}
} // namespace star::service::detail::queue_ownership