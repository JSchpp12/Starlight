#pragma once

#include "core/device/managers/ManagerEventBusTies.hpp"

#include "core/device/system/event/ManagerRequest.hpp"
#include "device/managers/Manager.hpp"

namespace star::core::device::manager
{
struct FenceRequest
{
    bool createSignalState = false;
};
struct FenceRecord
{
    vk::Fence fence;
    bool isReady() const
    {
        return true;
    }
    void cleanupRender(device::StarDevice &device)
    {
        device.getVulkanDevice().destroyFence(fence);
    }
};
constexpr std::string FenceEventName()
{
    return "fence_event_callback";
}
class Fence : public ManagerEventBusTies<FenceRecord, FenceRequest, 50>
{
  public:
    Fence()
        : ManagerEventBusTies<FenceRecord, FenceRequest, 50>(common::special_types::FenceTypeName, FenceEventName())
    {
    }
    virtual ~Fence() = default;

  protected:
    virtual FenceRecord createRecord(device::StarDevice &device, FenceRequest &&request) const
    {
        return FenceRecord{
            .fence = device.getVulkanDevice().createFence(vk::FenceCreateInfo().setFlags(
                request.createSignalState ? vk::FenceCreateFlagBits::eSignaled : vk::FenceCreateFlagBits()))};
    }
};
} // namespace star::core::device::manager