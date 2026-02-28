#pragma once

#include "core/device/system/event/ManagerRequest.hpp"
#include "device/managers/ManagerEventBusTies.hpp"
#include <star_common/EventBus.hpp>

#include <vulkan/vulkan.h>

namespace star::core::device::manager
{
struct SemaphoreRequest
{
    std::optional<uint64_t> initialSignalValue;
    bool isTimelineSemaphore;

    /// Triggers creation of binary semaphore
    SemaphoreRequest() : initialSignalValue(std::nullopt), isTimelineSemaphore(false)
    {
    }

    /// Trigger creation of timeline semaphore
    explicit SemaphoreRequest(uint64_t initialSignalValue)
        : initialSignalValue(std::make_optional(std::move(initialSignalValue))), isTimelineSemaphore(true)
    {
    }

    explicit SemaphoreRequest(bool isTimeline)
        : initialSignalValue(isTimeline ? std::make_optional(0) : std::nullopt), isTimelineSemaphore(isTimeline)
    {
    }
};

struct SemaphoreRecord
{
    vk::Semaphore semaphore = VK_NULL_HANDLE;
    std::optional<uint64_t> timelineValue = std::nullopt;

    bool isTimelineSemaphore() const
    {
        return timelineValue.has_value();
    }
    bool isReady() const
    {
        return true;
    }
    void cleanupRender(device::StarDevice &device)
    {
        device.getVulkanDevice().destroySemaphore(semaphore);
        semaphore = VK_NULL_HANDLE;
        timelineValue = std::nullopt;
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