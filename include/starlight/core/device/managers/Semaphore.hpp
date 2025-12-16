#pragma once

#include "core/device/system/event/ManagerRequest.hpp"
#include "device/managers/ManagerEventBusTies.hpp"
#include <star_common/EventBus.hpp>

namespace star::core::device::manager
{
struct SemaphoreRequest
{
    bool isTimelineSemaphore;
};

struct SemaphoreRecord
{
    vk::Semaphore semaphore;
    bool isTimelineSemaphore;
    bool isReady() const
    {
        return true;
    }
    void cleanupRender(device::StarDevice &device)
    {
        device.getVulkanDevice().destroySemaphore(semaphore);
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
    virtual SemaphoreRecord createRecord(SemaphoreRequest &&request) const override
    {
        return SemaphoreRecord{.semaphore = CreateSemaphore(*this->m_device, request.isTimelineSemaphore),
                               .isTimelineSemaphore = request.isTimelineSemaphore};
    }

  private:
    static vk::Semaphore CreateSemaphore(device::StarDevice &device, const bool &isTimelineSemaphore);
};
} // namespace star::core::device::manager