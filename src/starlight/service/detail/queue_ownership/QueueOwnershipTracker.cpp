#include "starlight/service/detail/queue_ownership/QueueOwnershipTracker.hpp"

namespace star::service::detail::queue_ownership
{
QueueOwnershipTracker::QueueOwnershipTracker(std::vector<StarQueueFamily> reqQueueFamilies)
    : m_regQueueFamilies(std::move(reqQueueFamilies)),
      isQueueAvailable(std::vector<std::vector<bool>>(m_regQueueFamilies.size()))
{
    for (size_t i = 0; i < m_regQueueFamilies.size(); i++)
    {
        this->isQueueAvailable.at(i) = std::vector<bool>(m_regQueueFamilies.at(i).getQueueCount(), true);
    }
}
std::optional<StarQueue> QueueOwnershipTracker::giveMeQueueWithProperties(const vk::QueueFlags &capabilities,
                                                                          const bool &presentationSupport,
                                                                          const uint32_t &familyIndex)
{
    for (size_t i = 0; i < m_regQueueFamilies.size(); i++)
    {
        if (this->m_regQueueFamilies.at(i).getQueueFamilyIndex() == familyIndex &&
            this->m_regQueueFamilies.at(i).doesSupport(capabilities, presentationSupport))
        {
            for (size_t j = 0; j < this->isQueueAvailable.at(i).size(); j++)
            {
                if (this->isQueueAvailable.at(i).at(j))
                {
                    this->isQueueAvailable.at(i).at(j) = false;
                    return std::make_optional(m_regQueueFamilies.at(i).getQueues().at(j));
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

    for (size_t i = 0; i < m_regQueueFamilies.size(); i++)
    {
        if (m_regQueueFamilies.at(i).doesSupport(capabilities, presentationSupport))
        {
            indices.push_back(m_regQueueFamilies.at(i).getQueueFamilyIndex());
        }
    }

    return indices;
}

std::vector<uint32_t> QueueOwnershipTracker::getAllQueueFamilyIndices() const
{
    std::vector<uint32_t> indices = std::vector<uint32_t>(m_regQueueFamilies.size());
    for (size_t i{0}; m_regQueueFamilies.size(); i++)
    {
        indices[i] = m_regQueueFamilies.at(i).getQueueFamilyIndex();
    }

    return indices;
}
} // namespace star::service::detail::queue_ownership