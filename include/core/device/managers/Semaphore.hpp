#pragma once

#include "core/device/system/EventBus.hpp"
#include "core/device/system/event/CreateSemaphore.hpp"
#include "device/managers/Manager.hpp"


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

class Semaphore : public Manager<SemaphoreRecord, SemaphoreRequest, Handle_Type::semaphore, 200>
{
  protected:
    SemaphoreRecord createRecord(device::StarDevice & device, SemaphoreRequest && request) const override
    {
        return SemaphoreRecord{.semaphore = CreateSemaphore(device, request.isTimelineSemaphore),
                               .isTimelineSemaphore = request.isTimelineSemaphore};
    }

  private:
    std::shared_ptr<StarDevice> m_device;
    static vk::Semaphore CreateSemaphore(device::StarDevice &device, const bool &isTimelineSemaphore);
};
} // namespace star::core::device::manager