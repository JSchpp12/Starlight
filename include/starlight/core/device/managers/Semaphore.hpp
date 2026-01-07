#pragma once

#include "core/device/system/event/ManagerRequest.hpp"
#include "device/managers/ManagerEventBusTies.hpp"
#include <star_common/EventBus.hpp>

#include <vulkan/vulkan.h>

namespace star::core::device::manager
{
struct SemaphoreRequest
{
    bool isTimelineSemaphore;
};

struct SemaphoreRecord
{
    vk::Semaphore semaphore = VK_NULL_HANDLE;
    std::optional<uint64_t> timlineValue = std::nullopt;

    bool isTimelineSemaphore() const
    {
        return timlineValue.has_value();
    }
    bool isReady() const
    {
        return true;
    }
    void cleanupRender(device::StarDevice &device)
    {
        device.getVulkanDevice().destroySemaphore(semaphore);
        semaphore = VK_NULL_HANDLE;
        timlineValue = std::nullopt;
    }
};

constexpr std::string_view GetSemaphoreEventTypeName = "semaphore_event_callback";
class Semaphore : public ManagerEventBusTies<SemaphoreRecord, SemaphoreRequest, 3000>
{
  public:
    Semaphore()
        : ManagerEventBusTies<SemaphoreRecord, SemaphoreRequest, 3000>(common::special_types::SemaphoreTypeName,
                                                                       GetSemaphoreEventTypeName)
    {
    }
    virtual ~Semaphore() = default;

  protected:
    virtual SemaphoreRecord createRecord(SemaphoreRequest &&request) const override;
};
} // namespace star::core::device::manager