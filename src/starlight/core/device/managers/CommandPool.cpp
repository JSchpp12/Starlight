#include "starlight/core/device/managers/CommandPool.hpp"

namespace star::core::device::manager
{
CommandPoolRecord CommandPool::createRecord(CommandPoolRequest &&request) const
{
    return {StarCommandPool(this->m_device->getVulkanDevice(), request.familyIndex, request.setAutoReset)};
}
} // namespace star::core::device::manager