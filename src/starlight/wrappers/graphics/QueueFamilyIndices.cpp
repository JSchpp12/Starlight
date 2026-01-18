#include "starlight/wrappers/graphics/QueueFamilyIndices.hpp"

void star::QueueFamilyIndices::registerFamily(const uint32_t &familyIndex, const vk::QueueFlags &queueSupport,
                                               const vk::Bool32 &presentSupport, const uint32_t &familyQueueCount)
{
    this->familyIndexQueueCount[familyIndex] = familyQueueCount;
    this->familyIndexQueueSupport[familyIndex] = queueSupport;
    this->allIndicies.insert(familyIndex);

    if (presentSupport)
        this->presentFamilies.insert(familyIndex);

    if (queueSupport & vk::QueueFlagBits::eGraphics)
        this->graphicsFamilies.insert(familyIndex);

    if (queueSupport & vk::QueueFlagBits::eTransfer)
        this->transferFamilies.insert(familyIndex);

    if (queueSupport & vk::QueueFlagBits::eCompute)
        this->computeFamilies.insert(familyIndex);
}

bool star::QueueFamilyIndices::isOptimalSupport(const bool needsPresentationSupport) const
{
    if (!this->isFullySupported(needsPresentationSupport))
    {
        return false;
    }

    std::vector<uint32_t> base{};
    // select presentation
    if (needsPresentationSupport)
    {
        // select queue with present + graphics

        std::vector<uint32_t> found;
        std::set_intersection(this->graphicsFamilies.begin(), this->graphicsFamilies.end(),
                              this->presentFamilies.begin(), this->presentFamilies.end(), std::back_inserter(found));
        if (found.size() > 0)
            base.push_back(found[0]);
        else
        {
            return false;
        }
    }
    else
    {
        auto it = graphicsFamilies.begin();
        if (it != graphicsFamilies.end())
        {
            base.push_back(*it);
        }
        else
        {
            return false;
        }
    }

    // select compute
    {
        std::vector<uint32_t> found;
        std::set_difference(this->computeFamilies.begin(), this->computeFamilies.end(), base.begin(), base.end(),
                            std::back_inserter(found));
        if (found.size() > 0)
            base.push_back(found[0]);
        else
            return false;
    }

    // select transfer
    {
        std::vector<uint32_t> found;
        std::set_difference(this->transferFamilies.begin(), this->transferFamilies.end(), base.begin(), base.end(),
                            std::back_inserter(found));
        if (found.size() > 0)
            base.push_back(found[0]);
        else
            return false;
    }

    return true;
}

bool star::QueueFamilyIndices::isFullySupported(const bool needsPresentationSupport) const
{
    if (needsPresentationSupport)
    {
        return this->graphicsFamilies.size() > 0 && this->transferFamilies.size() > 0 &&
               this->computeFamilies.size() > 0 && this->presentFamilies.size() > 0;
    }

    return this->graphicsFamilies.size() > 0 && this->transferFamilies.size() > 0 && this->computeFamilies.size() > 0;
}

bool star::QueueFamilyIndices::isSuitable(const bool needsPresentationSupport) const
{
    if (needsPresentationSupport)
    {
        return graphicsFamilies.size() > 0 && presentFamilies.size() > 0;
    }
    return graphicsFamilies.size() > 0;
}

std::vector<star::StarQueueFamily> star::QueueFamilyIndices::getQueueFamilies()
{
    std::vector<StarQueueFamily> queueFamilies = std::vector<StarQueueFamily>();
    for (auto uniqueIndex : getUniques())
    {
        queueFamilies.emplace_back(uniqueIndex, getNumQueuesForIndex(uniqueIndex),
                                   getSupportForIndex(uniqueIndex),
                                   getSupportsPresentForIndex(uniqueIndex));
    }

    return queueFamilies;
}