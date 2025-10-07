#pragma once

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
    bool isReady() const{
        return true; 
    }
    void cleanupRender(device::StarDevice &device){
        device.getVulkanDevice().destroySemaphore(semaphore);
    }
};

class Semaphore : public Manager<SemaphoreRecord, SemaphoreRequest, 200>
{
  protected:
    Handle_Type getHandleType() const override
    {
        return Handle_Type::semaphore;
    }

    SemaphoreRecord createRecord(device::StarDevice &device, SemaphoreRequest &&request) const override
    {
        return SemaphoreRecord{
            .semaphore = CreateSemaphore(device, request.isTimelineSemaphore),
            .isTimelineSemaphore = request.isTimelineSemaphore
        };
    }

  private:
    static vk::Semaphore CreateSemaphore(device::StarDevice &device, const bool &isTimelineSemaphore); 

};
} // namespace star::core::device::manager