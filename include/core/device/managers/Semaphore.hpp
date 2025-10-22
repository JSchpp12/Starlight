#pragma once

#include "core/device/system/EventBus.hpp"
#include "core/device/system/event/ManagerRequest.hpp"
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

class Semaphore : public Manager<SemaphoreRecord, SemaphoreRequest, Handle_Type::semaphore, 200>,
                  public std::enable_shared_from_this<Semaphore>
{
  public:
    void init(std::shared_ptr<device::StarDevice> device, core::device::system::EventBus &bus)
    {
        m_device = std::move(device); 

        auto weakSelf = weak_from_this();
        bus.subscribe<core::device::system::event::ManagerRequest<SemaphoreRequest>>(
            [weakSelf](const core::device::system::EventBase &e, bool &keepAlive) {
                if (auto self = weakSelf.lock()){
                    const auto &semaphoreEvent = static_cast<const core::device::system::event::ManagerRequest<SemaphoreRequest>&>(e);
                    auto handle = self->insert(*self->m_device, semaphoreEvent.giveMeRequest()); 

                    semaphoreEvent.getResultingHandle() = handle;
                }

                keepAlive = true; 
            });
    }

  protected:
    SemaphoreRecord createRecord(device::StarDevice &device, SemaphoreRequest &&request) const override
    {
        return SemaphoreRecord{.semaphore = CreateSemaphore(device, request.isTimelineSemaphore),
                               .isTimelineSemaphore = request.isTimelineSemaphore};
    }

  private:
    std::shared_ptr<StarDevice> m_device;
    static vk::Semaphore CreateSemaphore(device::StarDevice &device, const bool &isTimelineSemaphore);
};
} // namespace star::core::device::manager