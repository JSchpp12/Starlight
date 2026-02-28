#include "device/managers/Semaphore.hpp"

#include "starlight/core/Exceptions.hpp"

namespace star::core::device::manager
{
static inline vk::Semaphore CreateSemaphore(device::StarDevice &device, bool isTimelineSemaphore,
                                            std::optional<uint64_t> initialSignaledValue)
{
    vk::SemaphoreTypeCreateInfo typeInfo =
        vk::SemaphoreTypeCreateInfo()
            .setSemaphoreType(isTimelineSemaphore ? vk::SemaphoreType::eTimeline : vk::SemaphoreType::eBinary)
            .setInitialValue(initialSignaledValue.has_value() ? initialSignaledValue.value() : 0);

    vk::SemaphoreCreateInfo createInfo = vk::SemaphoreCreateInfo().setPNext(&typeInfo);

    vk::Semaphore newSemaphore = device.getVulkanDevice().createSemaphore(createInfo);

    if (newSemaphore == VK_NULL_HANDLE)
    {
        STAR_THROW("Failed to create semaphore");
    }

    return newSemaphore;
}

SemaphoreRecord Semaphore::createRecord(SemaphoreRequest &&request) const
{
    std::optional<uint64_t> timelineValue = std::nullopt;
    if (request.isTimelineSemaphore)
    {
        timelineValue = request.initialSignalValue.has_value() ? request.initialSignalValue.value() : 0;
    }
    
    return SemaphoreRecord{
        .semaphore = CreateSemaphore(*this->m_device, request.isTimelineSemaphore, request.initialSignalValue),
        .timelineValue = timelineValue};
}
} // namespace star::core::device::manager