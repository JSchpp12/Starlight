#pragma once

#include "StarQueueFamily.hpp"

#include <optional>
#include <vector>

namespace star::service::detail::queue_ownership
{
class QueueOwnershipTracker
{
  public:
    QueueOwnershipTracker() = default;
    explicit QueueOwnershipTracker(std::vector<StarQueueFamily> regQueueFamilies);

    std::optional<StarQueue> giveMeQueueWithProperties(const vk::QueueFlags &capabilities,
                                                       const bool &presentationSupport, const uint32_t &familyIndex);
    std::vector<uint32_t> getAllQueueFamilyIndices() const;
    std::vector<uint32_t> getQueueFamiliesWhichSupport(const vk::QueueFlags &capabilities,
                                                       const bool presentationSupport = false);

  private:
    std::vector<StarQueueFamily> m_regQueueFamilies = std::vector<StarQueueFamily>();
    std::vector<std::vector<bool>> isQueueAvailable = std::vector<std::vector<bool>>();
};
} // namespace star::service::detail::queue_ownership