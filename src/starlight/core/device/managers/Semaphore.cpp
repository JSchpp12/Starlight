#include "device/managers/Semaphore.hpp"

namespace star::core::device::manager
{
static inline vk::Semaphore CreateSemaphore(device::StarDevice &device, bool isTimelineSemaphore)
{
    vk::SemaphoreTypeCreateInfo typeInfo =
        vk::SemaphoreTypeCreateInfo()
            .setSemaphoreType(isTimelineSemaphore ? vk::SemaphoreType::eTimeline : vk::SemaphoreType::eBinary)
            .setInitialValue(0);

    vk::SemaphoreCreateInfo createInfo = vk::SemaphoreCreateInfo().setPNext(&typeInfo);

    vk::Semaphore newSemaphore = device.getVulkanDevice().createSemaphore(createInfo);

    if (newSemaphore == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to create semaphore");
    }

    return newSemaphore;
}

SemaphoreRecord Semaphore::createRecord(SemaphoreRequest &&request) const
{
    return SemaphoreRecord{.semaphore = CreateSemaphore(*this->m_device, request.isTimelineSemaphore),
                           .timlineValue =
                               request.isTimelineSemaphore ? std::make_optional<uint64_t>(0) : std::nullopt};
}
} // namespace star::core::device::manager